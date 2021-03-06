// This file belongs to the "MiniCore" game engine.
// Copyright (C) 2013 Jussi Lind <jussi.lind@iki.fi>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301, USA.
//

#ifndef MCMESHMETADATA_HH
#define MCMESHMETADATA_HH

#include <MCGLColor>
#include <MCVector3d>

#include <map>
#include <string>

/*! Mesh metadata structure returned by MCMeshConfigLoader.
 *  MCMeshManager can create MCMesh objects based on this data. */
struct MCMeshMetaData
{
    MCMeshMetaData()
    {}

    //! Handle of the mesh.
    std::string handle;

    //! .obj-file path.
    std::string modelPath;

    //! Texture/surface handle (see MCSurfaceManager).
    std::string texture1;

    //! Texture/surface handle (see MCSurfaceManager).
    std::string texture2;

    //! Color
    MCGLColor color;

    //! Scale
    std::pair<MCVector3dF, bool> scale;
};

#endif // MCMESHMETADATA_HH
