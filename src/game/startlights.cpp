// This file is part of Dust Racing (DustRAC).
// Copyright (C) 2012 Jussi Lind <jussi.lind@iki.fi>
//
// DustRAC is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// DustRAC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with DustRAC. If not, see <http://www.gnu.org/licenses/>.

#include "startlights.hpp"
#include "inputhandler.hpp"

Startlights::Startlights(InputHandler & inputHandler)
  : m_state(LightsInit)
  , m_counter(0)
  , m_stepsPerState(100)
  , m_inputHandler(inputHandler)
{
}

bool Startlights::updateCounter(MCUint limit)
{
    if (++m_counter > limit)
    {
        m_counter = 0;
        return true;
    }
    return false;
}

void Startlights::update()
{
    const MCUint second = m_stepsPerState;
    switch (m_state)
    {
    case LightsInit:
        m_pos = MCVector3dF(m_width / 2, 3 * m_height / 2, 0);
        m_animation.init(
             m_pos,
             m_pos,
             MCVector3dF(m_pos.i(), m_height / 2, 0),
             second / 3);
        m_state = LightsOff;
        m_inputHandler.setEnabled(false);
        break;

    case LightsOff:
        m_animation.update();
        if (updateCounter(second))
        {
            m_state = LightsFirstRow;
        }
        break;

    case LightsFirstRow:
        if (updateCounter(second))
        {
            m_state = LightsSecondRow;
        }
        break;

    case LightsSecondRow:
        if (updateCounter(second))
        {
            m_state = LightsThirdRow;
        }
        break;

    case LightsThirdRow:
        if (updateCounter(second))
        {
            m_state = LightsGo;
            m_inputHandler.setEnabled(true);
        }
        break;

    case LightsGo:
        if (updateCounter(second))
        {
            m_state = LightsEnd;
            m_animation.init(m_pos,
                             m_pos,
                             MCVector3dF(m_pos.i(), 3 * m_height / 2, 0),
                             second / 3);
        }
        break;

    case LightsEnd:
        m_animation.update();
        break;
    }
}

void Startlights::setDimensions(MCUint width, MCUint height)
{
    m_width  = width;
    m_height = height;
}

Startlights::LightState Startlights::state() const
{
    return m_state;
}

const MCVector3dF & Startlights::pos() const
{
    return m_pos;
}