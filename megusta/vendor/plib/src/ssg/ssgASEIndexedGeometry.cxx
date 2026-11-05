//
// ASE ( 3DSMAX ASCII EXPORT Version 2.00 ) loader for SSG/PLIB
// Written by Dave McClurg (dpm@efn.org) in Feb-2000
//

// Adapted indexed-remap fragment. Original licenses are retained in vendor/plib;
// provenance is in manifests/evidence. This is maintained implementation source.
#include "ssgASX.h"
#include "megusta_ase_support.h"
#include <stdexcept>

namespace megusta::asx {
Geometry build_indexed_geometry(aseMesh* mesh, aseMaterial* mat) {
  u32 i;
// BEGIN ORIGINAL-MATCHED fragment: PLIB 1.3.1 add_mesh / EXE 00463f90.
  //allocate map_index array
  int* map_index = new int [ mesh -> num_faces * 3 ] ;

  //allocate the vertex list
  u32 max_verts = mesh -> num_verts + mesh -> num_faces * 3 ;
  aseVertexBuffer* vert_list = new aseVertexBuffer [ max_verts ] ;
  
  //mark each vertex as *not* used
  aseVertexBuffer* vert = vert_list ;
  for ( i=0; i < max_verts; i++, vert++ )
  {
    vert -> use_flag = false ;
  }

  u32 extra_verts = 0 ;

  //build the vertex list
  aseFace* face = mesh -> faces ;
  for ( i=0; i<mesh -> num_faces; i++, face++ )
  {
    if ( mat->sub_flag && face->sub_index != mat->sub_index )
      continue ;
    
    for ( u32 j=0; j<3; j++ )
    {
      int k = i*3+j;

      map_index [k] = face->v[j] ;
      vert = vert_list + map_index[k];

      if ( vert -> use_flag )
      {
        //check for match
        bool match = true ;
        // MG-PLIB-083: the recovered comparisons are ordered equality;
        // unlike sgCompareVec* with tolerance zero, NaNs do not match.
        if ( mesh -> tverts &&
          !(vert->tv[0] == mesh->tverts[face->tv[j]][0] &&
            vert->tv[1] == mesh->tverts[face->tv[j]][1]) )
          match = false ;
        else if ( mesh -> cverts &&
          !(vert->cv[0] == mesh->cverts[face->cv[j]][0] &&
            vert->cv[1] == mesh->cverts[face->cv[j]][1] &&
            vert->cv[2] == mesh->cverts[face->cv[j]][2]) )
          match = false ;
        if ( match )
          continue ;  //texcoord and color matches other vertex

        extra_verts ++ ;

        map_index [k] = mesh -> num_verts + k ;
        vert = vert_list + map_index[k];
      }

      //add the vertex
      vert -> use_flag = true;
      sgCopyVec3 ( vert -> v, mesh -> verts[ face->v[j] ] ) ;
      if ( mesh -> tverts )
        sgCopyVec2 ( vert -> tv, mesh -> tverts[ face->tv[j] ] ) ;
      if ( mesh -> cverts )
        sgCopyVec3 ( vert -> cv, mesh -> cverts[ face->cv[j] ] ) ;
    }
  }

  //assign a unique index to each vertex
  int num_verts = 0 ;
  vert = vert_list;
  for ( i=0; i < max_verts; i++, vert++ )
  {
    if ( vert -> use_flag )
      vert -> index = num_verts ++;
  }
    
  //if ( extra_verts > 0 )
  //   fprintf( stdout, "%d verts; %d added\n", num_verts-extra_verts, extra_verts );
  //else
  //   fprintf( stdout, "%d verts\n", num_verts );

// END ORIGINAL-MATCHED fragment.
// RECONSTRUCTED-EVIDENCE-DERIVED output adapter: EXE indexed path.
  (void)extra_verts;
  Geometry output;
  if (num_verts > 65536) {
    delete[] vert_list; delete[] map_index;
    throw std::runtime_error("16-bit geometry index overflow");
  }
  for (u32 f=0;f<mesh->num_faces;++f) {
    if (mat->sub_flag && mesh->faces[f].sub_index!=mat->sub_index) continue;
    for (u32 j=0;j<3;++j)
      output.indices.push_back(static_cast<uint16_t>(vert_list[map_index[f*3+j]].index));
  }
  for (i=0;i<max_verts;++i) {
    auto& v=vert_list[i]; if (!v.use_flag) continue;
    output.vertices.push_back({v.v[0],v.v[1],v.v[2]});
    if (mesh->tverts){
      sgVec2 uv={v.tv[0],v.tv[1]};
      _ssgAseTexCoord(uv,mat->texrep,mat->texoff);
      output.texcoords.push_back({uv[0],uv[1]});
    }
    if (mesh->cverts) output.colours.push_back({v.cv[0],v.cv[1],v.cv[2],_ssgUseMaterialTransparency ? float(1.0f-mat->floats[17]) : 1.0f}); // MG-PLIB-073
  }
  delete[] vert_list; delete[] map_index;
  return output;
}

} // namespace megusta::asx
