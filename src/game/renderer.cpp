// This file is part of Dust Racing 2D.
// Copyright (C) 2011 Jussi Lind <jussi.lind@iki.fi>
//
// Dust Racing 2D is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Dust Racing 2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dust Racing 2D. If not, see <http://www.gnu.org/licenses/>.

#include "renderer.hpp"

#include "eventhandler.hpp"
#include "fontfactory.hpp"
#include "scene.hpp"

#ifdef __MC_GL30__
#include "shaders30.h"
#else
#include "shaders.h" // These are also for GLES now
#endif

#include "../common/config.hpp"

#include <MCGLScene>
#include <MCAssetManager>
#include <MCException>
#include <MCLogger>
#include <MCSurface>
#include <MCSurfaceManager>
#include <MCTrigonom>
#include <MCWorld>

#include <cmath>
#include <cassert>

#include <QApplication>
#include <QDesktopWidget>
#include <QDir>
#include <QFontDatabase>
#include <QIcon>
#include <QKeyEvent>
#include <QOpenGLFramebufferObject>
#include <QScreen>

Renderer * Renderer::m_instance = nullptr;

Renderer::Renderer(int hRes, int vRes, bool fullScreen)
: m_context(nullptr)
, m_scene(nullptr)
, m_glScene(new MCGLScene)
, m_eventHandler(nullptr)
, m_viewAngle(45.0)
, m_fadeValue(1.0)
, m_enabled(false)
, m_hRes(hRes)
, m_vRes(vRes)
, m_fullHRes(QGuiApplication::primaryScreen()->geometry().width())
, m_fullVRes(QGuiApplication::primaryScreen()->geometry().height())
, m_fullScreen(fullScreen)
, m_updatePending(false)
{
    assert(!Renderer::m_instance);
    Renderer::m_instance = this;

    setSurfaceType(QWindow::OpenGLSurface);

    setTitle(QString(Config::Game::GAME_NAME) + " " + Config::Game::GAME_VERSION);
    setIcon(QIcon(":/dustrac-game.png"));
}

Renderer & Renderer::instance()
{
    assert(Renderer::m_instance);
    return *Renderer::m_instance;
}

void Renderer::initialize()
{
    MCLogger().info() << "OpenGL Version: " << glGetString(GL_VERSION);

    if (!m_fullScreen)
    {
        // Set window size & disable resize
        resize(m_hRes, m_vRes);
        setMinimumSize(QSize(m_hRes, m_vRes));
        setMaximumSize(QSize(m_hRes, m_vRes));

        // Try to center the window
        setPosition(m_fullHRes / 2 - m_hRes / 2, m_fullVRes / 2 - m_vRes / 2);
    }

    m_glScene->initialize();

    resizeGL(m_hRes, m_vRes);

    loadShaders();
    loadFonts();

    emit initialized();
}

void Renderer::resizeGL(int viewWidth, int viewHeight)
{
    m_glScene->resize(
        viewWidth, viewHeight, Scene::width(), Scene::height(),
        m_viewAngle);
}

void Renderer::createProgramFromSource(std::string handle, std::string vshSource, std::string fshSource)
{
    // Inject precision qualifiers
#ifdef __MC_GLES__
    QString origVsh(vshSource.c_str());
    origVsh.replace("#version 120", "#version 100\nprecision mediump float;\nprecision mediump int;\n");
    vshSource = origVsh.toStdString();

    QString origFsh(fshSource.c_str());
    origFsh.replace("#version 120", "#version 100\nprecision mediump float;\nprecision mediump int;\n");
    fshSource = origFsh.toStdString();
#endif

    // Note: ShaderProgram throws on error.
    MCGLShaderProgram * program = new MCGLShaderProgram(vshSource, fshSource);
    m_shaderHash[handle].reset(program);
}

void Renderer::loadShaders()
{
    // Engine defaults
    m_shaderHash["default"]             = MCGLScene::instance().defaultShaderProgram();
    m_shaderHash["defaultSpecular"]     = MCGLScene::instance().defaultSpecularShaderProgram();
    m_shaderHash["defaultShadow"]       = MCGLScene::instance().defaultShadowShaderProgram();
    m_shaderHash["particle"]            = MCGLScene::instance().defaultParticleShaderProgram();
    m_shaderHash["pointParticle"]       = MCGLScene::instance().defaultPointParticleShaderProgram();
    m_shaderHash["pointParticleRotate"] = MCGLScene::instance().defaultPointParticleRotateShaderProgram();
    m_shaderHash["text"]                = MCGLScene::instance().defaultTextShaderProgram();
    m_shaderHash["textShadow"]          = MCGLScene::instance().defaultTextShadowShaderProgram();

    // Custom shaders
    createProgramFromSource("car",    carVsh,  carFsh);
    createProgramFromSource("fbo",    fboVsh,  fboFsh);
    createProgramFromSource("menu",   menuVsh, MCGLShaderProgram::getDefaultFragmentShaderSource());
    createProgramFromSource("tile2d", tileVsh, MCGLShaderProgram::getDefaultFragmentShaderSource());
    createProgramFromSource("tile3d", tileVsh, tile3dFsh);
}

void Renderer::loadFonts()
{
    const std::vector<QString> fonts = {"UbuntuMono-R.ttf", "UbuntuMono-B.ttf"};
    for (auto font : fonts)
    {
        const QString path =
            QString(Config::Common::dataPath) + QDir::separator() + "fonts" + QDir::separator() + font;
        MCLogger().info() << "Loading font " << path.toStdString() << "..";

        QFile fontFile(path);
        fontFile.open(QFile::ReadOnly);
        const int appFontId = QFontDatabase::addApplicationFontFromData(fontFile.readAll());
        if (appFontId < 0)
        {
            MCLogger().warning() << "Failed to load font " << path.toStdString() << "..";
        }
    }

    MCAssetManager::instance().textureFontManager().createFontFromData(FontFactory::generateFont());
}

void Renderer::setEnabled(bool enable)
{
    m_enabled = enable;
}

MCGLShaderProgramPtr Renderer::program(const std::string & id)
{
    MCGLShaderProgramPtr program(m_shaderHash[id]);
    if (!program)
    {
        throw MCException("Cannot find shader program '" + id + "'");
    }
    return program;
}

MCGLScene & Renderer::glScene()
{
    return *m_glScene;
}

void Renderer::setFadeValue(float value)
{
    m_fadeValue = value;
}

float Renderer::fadeValue() const
{
    return m_fadeValue;
}

void Renderer::render()
{
    if (!m_scene)
    {
        return;
    }

    // Render the game scene to the frame buffer object
    resizeGL(m_hRes, m_vRes);

    static QOpenGLFramebufferObject fbo(m_hRes, m_vRes);
    static QOpenGLFramebufferObject shadowFbo(m_hRes, m_vRes);
    static MCGLMaterialPtr dummyMaterial(new MCGLMaterial);

    shadowFbo.bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_scene->renderObjectShadows();

    shadowFbo.release();

    fbo.bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_scene->renderTrack();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    dummyMaterial->setTexture(shadowFbo.texture(), 0);
    MCSurface ss(dummyMaterial, 2.0f, 2.0f);
    ss.setShaderProgram(program("fbo"));
    ss.bindMaterial();
    ss.render(nullptr, MCVector3dF(), 0);

    glDisable(GL_BLEND);

    m_scene->renderObjects();

    m_scene->renderCommonHUD();

    fbo.release();

    // Render the frame buffer object onto the screen
    if (m_fullScreen)
    {
        resizeGL(m_fullHRes, m_fullVRes);
    }
    else
    {
        resizeGL(m_hRes, m_vRes);
    }

    dummyMaterial->setTexture(fbo.texture(), 0);
    MCSurface sd(dummyMaterial, 2.0f, 2.0f);
    sd.setShaderProgram(program("fbo"));
    sd.bindMaterial();
    sd.render(nullptr, MCVector3dF(), 0);
}

void Renderer::renderLater()
{
    if (!m_updatePending)
    {
        m_updatePending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool Renderer::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::UpdateRequest:
        m_updatePending = false;
        renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

void Renderer::exposeEvent(QExposeEvent *)
{
    if (isExposed())
    {
        renderNow();
    }
}

void Renderer::renderNow()
{
    if (!isExposed())
    {
        return;
    }

    bool needsInitialize = false;

    if (!m_context)
    {
        m_context = new QOpenGLContext(this);
        m_context->setFormat(requestedFormat());
        m_context->create();

        if (!m_context->isValid())
        {
            std::stringstream ss;
            ss << "Cannot create context for OpenGL version " <<
                  requestedFormat().majorVersion() << "." << requestedFormat().minorVersion();
            throw MCException(ss.str());
        }

        needsInitialize = true;
    }

    m_context->makeCurrent(this);

    if (needsInitialize)
    {
        initializeOpenGLFunctions();
        initialize();
    }

    render();

    m_context->swapBuffers(this);
}

void Renderer::resizeEvent(QResizeEvent * event)
{
    resizeGL(event->size().width(), event->size().height());
}

void Renderer::keyPressEvent(QKeyEvent * event)
{
    assert(m_eventHandler);
    if (!m_eventHandler->handleKeyPressEvent(event))
    {
        QWindow::keyPressEvent(event);
    }
}

void Renderer::keyReleaseEvent(QKeyEvent * event)
{
    assert(m_eventHandler);
    if (!m_eventHandler->handleKeyReleaseEvent(event))
    {
        QWindow::keyReleaseEvent(event);
    }
}

void Renderer::mousePressEvent(QMouseEvent * event)
{
    assert(m_eventHandler);
    m_eventHandler->handleMousePressEvent(event, width(), height(), true);
}

void Renderer::mouseReleaseEvent(QMouseEvent * event)
{
    assert(m_eventHandler);
    m_eventHandler->handleMouseReleaseEvent(event, width(), height(), true);
}

void Renderer::mouseMoveEvent(QMouseEvent * event)
{
    assert(m_eventHandler);
    m_eventHandler->handleMouseMoveEvent(event);
}

void Renderer::closeEvent(QCloseEvent * event)
{
    event->accept();
    emit closed();
}

void Renderer::setScene(Scene & scene)
{
    m_scene = &scene;
}

void Renderer::setEventHandler(EventHandler & eventHandler)
{
    m_eventHandler = &eventHandler;
}

Renderer::~Renderer()
{
    delete m_glScene;
}
