// Copyright (c) 2015-2016, Jeffrey N. Johnson
// All rights reserved.
// 
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include "cmocka.h"
#include "core/silo_file.h"
#include "polyglot/import_tetgen_mesh.h"

void test_import_tetgen_mesh(void** state)
{
  // Create a TetGen mesh from the tetgen_example.* files.
  mesh_t* mesh = import_tetgen_mesh(MPI_COMM_WORLD, 
                                    CMAKE_CURRENT_SOURCE_DIR "/tetgen_example.1.node", 
                                    CMAKE_CURRENT_SOURCE_DIR "/tetgen_example.1.ele", 
                                    CMAKE_CURRENT_SOURCE_DIR "/tetgen_example.1.face", 
                                    CMAKE_CURRENT_SOURCE_DIR "/tetgen_example.1.neigh");
  assert_true(mesh_verify_topology(mesh, polymec_error));

  int nprocs;
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  if (nprocs == 1)
  {
    assert_int_equal(1020, mesh->num_cells);
    assert_int_equal(0, mesh->num_ghost_cells);
    assert_int_equal(2286, mesh->num_faces);
    assert_int_equal(1569, mesh->num_edges);
    assert_int_equal(304, mesh->num_nodes);
  }
  else
  {
    int num_cells, num_ghost_cells, num_faces, num_edges, num_nodes;
    MPI_Allreduce(&mesh->num_cells, &num_cells, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&mesh->num_ghost_cells, &num_ghost_cells, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&mesh->num_faces, &num_faces, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&mesh->num_edges, &num_edges, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&mesh->num_nodes, &num_nodes, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    assert_true(num_cells == 1020);
    assert_true(num_ghost_cells > 0);
    assert_true(num_faces >= 2286);
    assert_true(num_edges >= 1569);
    assert_true(num_nodes >= 304);
  }
  mesh_free(mesh);
}

void test_plot_tetgen_mesh(void** state)
{
  // Create a TetGen mesh from the tetgen_example.* files.
  mesh_t* mesh = import_tetgen_mesh(MPI_COMM_WORLD, 
                                    CMAKE_CURRENT_SOURCE_DIR "/tetgen_example.1.node", 
                                    CMAKE_CURRENT_SOURCE_DIR "/tetgen_example.1.ele", 
                                    CMAKE_CURRENT_SOURCE_DIR "/tetgen_example.1.face", 
                                    CMAKE_CURRENT_SOURCE_DIR "/tetgen_example.1.neigh");
  // Plot it.
  int rank;
  MPI_Comm_rank(mesh->comm, &rank);
  double p[mesh->num_cells];
  for (int c = 0; c < mesh->num_cells; ++c)
    p[c] = 1.0*rank;
  silo_file_t* silo = silo_file_new(mesh->comm, "tetgen_example", "tetgen_example", 1, 0, 0, 0.0);
  silo_file_write_mesh(silo, "mesh", mesh);
  silo_file_write_scalar_cell_field(silo, "rank", "mesh", p, NULL);
  silo_file_close(silo);

  // Clean up.
  mesh_free(mesh);
}

int main(int argc, char* argv[]) 
{
  polymec_init(argc, argv);
  const struct CMUnitTest tests[] = 
  {
    cmocka_unit_test(test_import_tetgen_mesh),
    cmocka_unit_test(test_plot_tetgen_mesh)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
