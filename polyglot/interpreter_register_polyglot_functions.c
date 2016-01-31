// Copyright (c) 2015-2016, Jeffrey N. Johnson
// All rights reserved.
// 
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "polyglot/import_tetgen_mesh.h"
#include "polyglot/interpreter_register_polyglot_functions.h"
#include "polyglot/exodus_file.h"

// Lua stuff.
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

// This file contains definitions for functions supported by Polyglot.

// Tetgen mesh factory method.
static int mesh_factory_tetgen(lua_State* lua)
{
  // Check the arguments.
  int num_args = lua_gettop(lua);
  if ((num_args != 1) || !lua_isstring(lua, 1))
  {
    return luaL_error(lua, "Invalid argument(s). Usage:\n"
                      "mesh = mesh_factory.tetgen(mesh_prefix).");
  }

  // Use the mesh prefix to generate filenames.
  const char* mesh_prefix = lua_tostring(lua, 1);
  char node_file[512], ele_file[512], face_file[512], neigh_file[512];
  snprintf(node_file, 512, "%s.node", mesh_prefix);
  snprintf(ele_file, 512, "%s.ele", mesh_prefix);
  snprintf(face_file, 512, "%s.face", mesh_prefix);
  snprintf(neigh_file, 512, "%s.neigh", mesh_prefix);
  mesh_t* mesh = import_tetgen_mesh(MPI_COMM_WORLD, node_file, ele_file, 
                                    face_file, neigh_file);

  // Push the mesh onto the stack.
  lua_pushmesh(lua, mesh);
  return 1;
}

// read_exodus_mesh(args) -- This function reads a mesh from a the given 
// Exodus file on disk. 
static int lua_read_exodus_mesh(lua_State* lua)
{
  // Check the arguments.
  int num_args = lua_gettop(lua);
  if ((num_args != 1) || !lua_isstring(lua, 1))
  {
    return luaL_error(lua, "read_exodus_mesh: invalid arguments. Usage:\n"
                      "mesh = read_exodus_mesh(filename)");
  }

  // Get the argument(s).
  char* filename = string_dup(lua_tostring(lua, 1));

  // Do some checking.
  if (!file_exists(filename))
    return luaL_error(lua, "read_exodus_mesh: file does not exist.");
  size_t real_size;
  float version;
  int num_procs;
  bool valid = exodus_file_query(filename, &real_size, &version,
                                 &num_procs, NULL);
  if (!valid)
    return luaL_error(lua, "read_exodus_mesh: file contains an invalid mesh.");

  // Do our business.
  exodus_file_t* file = exodus_file_open(MPI_COMM_WORLD, filename);
  fe_mesh_t* fe_mesh = exodus_file_read_mesh(file);
  exodus_file_close(file);
  mesh_t* mesh = mesh_from_fe_mesh(fe_mesh);
  fe_mesh_free(fe_mesh);

  lua_pushmesh(lua, mesh);
  return 1;
}

void interpreter_register_polyglot_functions(interpreter_t* interp)
{
  if (!interpreter_has_global_table(interp, "mesh_factory"))
    interpreter_register_global_table(interp, "mesh_factory", NULL);
  interpreter_register_global_method(interp, "mesh_factory", "tetgen", mesh_factory_tetgen, NULL);
  interpreter_register_function(interp, "read_exodus_mesh", lua_read_exodus_mesh, NULL);
}

