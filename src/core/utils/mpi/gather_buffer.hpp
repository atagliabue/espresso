/*
  Copyright (C) 2017 The ESPResSo project
  Max-Planck-Institute for Polymer Research, Theory Group

  This file is part of ESPResSo.

  ESPResSo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ESPResSo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UTILS_MPI_GATHER_BUFFER_HPP
#define UTILS_MPI_GATHER_BUFFER_HPP

#include <algorithm>
#include <vector>

#include <boost/mpi/communicator.hpp>

namespace Utils {
namespace Mpi {

/**
 * @brief Gather buffer with differend size on each node.
 *
 * Gathers buffers with different lengths from all nodes to root.
 * The buffer is assumed to be large enough to hold the data from
 * all the nodes and is owned by the caller. On the root node no
 * data is copied, and the first n_elem elements of buffer are not
 * touched.
 *
 * @param buffer On the master the target buffer that has to be
          large enough to hold all elements and has the local
          part in the beginning. On the slaves the local buffer.
 * @param n_elem The number of elements in the local buffer.
 * @param The rank where the data should be gathered.
 * @return On rank root, the total number of elements in the buffer,
 *         on the other ranks 0.
 */
template <typename T>
int gather_buffer(T *buffer, int n_elem, boost::mpi::communicator comm,
                  int root = 0) {
  if (comm.rank() == root) {
    static std::vector<int> sizes;
    static std::vector<int> displ;
    sizes.resize(comm.size());
    displ.resize(comm.size());

    /* Gather sizes */
    MPI_Gather(&n_elem, 1, MPI_INT, sizes.data(), 1, MPI_INT, root, comm);

    /* Total logical size for return value */
    auto const tot_size = std::accumulate(sizes.begin(), sizes.end(), 0);

    int offset = 0;
    for (int i = 0; i < sizes.size(); i++) {
      /* Convert size from logical to physical */
      sizes[i] *= sizeof(T);
      displ[i] = offset;
      offset += sizes[i];
    }

    /* Gather data */
    MPI_Gatherv(MPI_IN_PLACE, 0, MPI_BYTE, buffer, sizes.data(), displ.data(),
                MPI_BYTE, root, comm);

    return tot_size;
  } else {
    /* Send local size */
    MPI_Gather(&n_elem, 1, MPI_INT, nullptr, 0, MPI_INT, root, comm);
    /* Send data */
    MPI_Gatherv(buffer, n_elem * sizeof(T), MPI_BYTE, nullptr, nullptr, nullptr,
                MPI_BYTE, root, comm);

    return 0;
  }
}

/**
 * @brief Gather buffer with differend size on each node.
 *
 * Gathers buffers with different lengths from all nodes to root.
 * The buffer is resized to the total size. On the root node no
 * data is copied, and the first n_elem elements of buffer are not
 * touched. On the slaves, the buffer is not touched.
 *
 * @param buffer On the master the target buffer that has the local
          part in the beginning. On the slaves the local buffer.
 * @param The rank where the data should be gathered.
 * @return On rank root, the total number of elements in the buffer,
 *         on the other ranks 0.
 */
template <typename T>
void gather_buffer(std::vector<T> &buffer, boost::mpi::communicator comm,
                   int root = 0) {
  auto const n_elem = buffer.size();

  if (comm.rank() == root) {
    static std::vector<int> sizes;
    sizes.resize(comm.size());

    /* Gather sizes */
    MPI_Gather(&n_elem, 1, MPI_INT, sizes.data(), 1, MPI_INT, root, comm);

    /* Total logical size */
    auto const tot_size = std::accumulate(sizes.begin(), sizes.end(), 0);

    /* Resize the buffer */
    buffer.resize(tot_size);

    static std::vector<int> displ;
    displ.resize(comm.size());

    int offset = 0;
    for (int i = 0; i < sizes.size(); i++) {
      /* Convert size from logical to physical */
      sizes[i] *= sizeof(T);
      displ[i] = offset;
      offset += sizes[i];
    }

    /* Gather data */
    MPI_Gatherv(MPI_IN_PLACE, 0, MPI_BYTE, buffer.data(), sizes.data(),
                displ.data(), MPI_BYTE, root, comm);
  } else {
    /* Send local size */
    MPI_Gather(&n_elem, 1, MPI_INT, nullptr, 0, MPI_INT, root, comm);
    /* Send data */
    MPI_Gatherv(buffer.data(), n_elem * sizeof(T), MPI_BYTE, nullptr, nullptr,
                nullptr, MPI_BYTE, root, comm);
  }
}
}
}

#endif
