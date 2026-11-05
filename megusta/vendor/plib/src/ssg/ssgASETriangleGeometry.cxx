/*
     PLIB - A Suite of Portable Game Libraries
     Copyright (C) 2001  Steve Baker
 
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
 
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
 
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the Free
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 
     For further information visit http://plib.sourceforge.net

     $Id: ssgLoadASE.cxx,v 1.35 2001/05/22 05:30:34 mcdave Exp $
*/

/****
* NAME
*   ssgLoadASE -- ASE model loader
*
* DESCRIPTION
*   ssgLoadASE will load an ASE model exported from 3dsmax using
*   the standard ascii export v2.00 plugin.
*
*   The material list and geometry objects are parsed and converted
*   into an SSG scene graph.  shape, light, camera, and helper
*   nodes are ignored.
*
*   If the geometry has vertex colours (pre-lit) then lighting is
*   disabled for that leaf.
*
*   mesh animation creates ssgSelector entities.  transform
*   animation creates ssgTransform entities with ssgTransformArray
*   user_data.  see the viewer example for details on controlling
*   animation.
*
*   NOTE: be sure you _reset the transform_ of the top-level object
*   in 3dsmax before exporting transform animation.
*
*   Please report any bugs to the author.  Complete information about
*   the ASE format can be found in the 3dsmax SDK source code.
*
* AUTHOR
*   Dave McClurg <dpm@efn.org>
*
* CREATION DATE
*   Feb-2000
*
* MODIFICATION HISTORY
*   November 2000 - v1.1, Dave McClurg <dpm@efn.org>
*     o support for transform animation samples
****/

// Maintained adapted upstream fragments plus reconstructed array adapters.
#include "ssgASX.h"
#include "megusta_ase_support.h"

namespace megusta::asx {
static void get_texcoord(aseMaterial* mat,sgVec2 uv)
{ _ssgAseTexCoord(uv,mat->texrep,mat->texoff); }
Geometry expand_triangle_geometry(aseMesh* mesh,aseMaterial* mat) {
    Geometry output;
    ArrayBuilder<3> vertex_adapter{output.vertices},normal_adapter{output.normals};
    ArrayBuilder<2> uv_adapter{output.texcoords};
    auto* vl=&vertex_adapter;auto* nl=&normal_adapter;auto* tl=&uv_adapter;
    u32 i;
// BEGIN ORIGINAL-MATCHED fragment: PLIB 1.3.2 add_mesh / EXE 00463f90.
    aseFace* face = mesh -> faces ;
    for ( i=0; i<mesh -> num_faces; i++, face++ )
    {
      if ( mat->sub_flag && face->sub_index != mat->sub_index )
        continue ;
      
      sgVec3 n ;
      sgMakeNormal ( n,
        mesh -> verts[ face->v[0] ] ,
        mesh -> verts[ face->v[1] ] ,
        mesh -> verts[ face->v[2] ] ) ;
      
      for ( u32 j=0; j<3; j++ )
      {
        vl -> add ( mesh -> verts[ face->v[j] ] ) ;
        nl -> add ( n ) ;
        
        if ( mesh -> tverts )
        {
          sgVec2 tv ;
          sgCopyVec2 ( tv, mesh -> tverts[ face->tv[j] ] ) ;
          get_texcoord ( mat, tv ) ;
          tl -> add ( tv ) ;
        }
      }
    }

// END ORIGINAL-MATCHED fragment.
    return output;
}

} // namespace megusta::asx
