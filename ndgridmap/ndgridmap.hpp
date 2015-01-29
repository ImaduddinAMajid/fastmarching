/*! \file ndgridmap.hpp
    \brief Templated class which represents a n dimensional grid map.
    
    Based on a flat vector in which the generalized indexing operations are efficiently
    implemented, according to this document [nDGridMaps](http://www.javiervgomez.com/index.php/ND_grid_maps)
    * It is important to read this document in order to understand the class.
    * 
    * It has 2 template parameters: the cells employed, should be Cell class or inherited.
    * number of dimensions of the grid. Helps compiler to optimize.
    * 
    Copyright (C) 2014 Javier V. Gomez and Jose Pardeiro
    www.javiervgomez.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NDGRIDMAP_H_
#define NDGRIDMAP_H_

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstddef>
#include <vector>

#include "../console/console.h"

    // TODO: a neighbors precomputation could speed things up.
    // TODO: improve coord2idx function in order to just pass n coordinates and not an vector.
    // TODO: overload [] and getCell methods to return const.

template <class T> class nDGridMap {

      /**
       Ostream operator << overload for this class.
       */
    friend std::ostream& operator <<
    (std::ostream & os, const nDGridMap<T> & g) {
        os << console::str_info("Grid cell information");
        os << "\t" << g.getCell(0).type() << std::endl;
        os << "\t" << g.ncells_ << " cells." << std::endl;
        os << "\t" << g.leafsize_ << " leafsize (m)." << std::endl;
        os << "\t" << g.ndims_ << " dimensions:" << std::endl;

        for (int i = 0; i < g.ndims_; ++i)
            os << "\t\t" << "d" << i << "\tsize: " << g.dimsize_[i] << std::endl;

        return os;
    }

    public:

      /**
       Constructor.
       *
       * @param dimsize constains the size of each dimension.
       * @param leafsize real cell size (assumed to be cubic). 1 unit by default.
       */
        nDGridMap
        (const std::vector<int> & dimsize, const float leafsize = 1.0f) :
        leafsize_(leafsize),
        clear_(true) {
            resize(dimsize);
        }

        /**
         * Resizes the grid.
         *
         * @param dimsize contains the size of each dimension.
         *
         * */
        void resize
        (const std::vector<int>& dimsize) {
            dimsize_ = dimsize;
            ndims_ = dimsize_.size();
            ncells_= 1;

            n_.reserve(ndims_);
            d_.reserve(ndims_);

            // Computing the total number of cells and the auxiliar vector d_.
            for (int i = 0; i < ndims_; ++i) {
                ncells_ *= dimsize_[i];
                d_[i] = ncells_;
            }

            //Resizing gridmap and initializing with default values.
            cells_.clear();
            cells_.resize(ncells_, T());

            // Setting the index_ member of the cells, which a-priori is unknown.
            for (int i = 0; i < cells_.size(); ++i)
                cells_[i].setIndex(i);
        }

        /**
         * Operator[] overload in order to access the elements of the grid map.
         *  grid[i] equivalent to grid.cells_[i] to grid.getCell(i);
         *
         * @param idx index of the cell to be accessed.
         *
         * @return the corresponding cell.
         *
         * @see getCell()
         *
         * */
        T & operator[]
        (const int idx) {
            return cells_[idx];
        }

        float getLeafSize() const { return leafsize_; }

        void setLeafSize(const float leafsize) { leafsize_ = leafsize; }

        /*
         * @see operator[]
         *
         * */
        T & getCell
        (const int idx) {
            return cells_[idx];
            }

        const std::vector<int>& getDimSizes() const     { return dimsize_;}

         /**
         * For a cell with index idx, obtains the minimum value of the neigbours in dimension dim.
         *
         * @param idx index of the cell accessed.
         * @param dim dimension in which the minimum is examinated (0: x, 1: y, 2: z, etc).
         *
         * @return the corresponding minimum value.
         * */
        double getMinValueInDim
        (const int idx, const int dim)   {
            n_neighs = 0; // How many neighbors obtained in that dimension.
            getNeighborsInDim(idx ,n_, dim);

            if (n_neighs == 1)
                return cells_[n_[0]].getValue();
            else
                return (cells_[n_[0]].getValue()<cells_[n_[1]].getValue()) ? cells_[n_[0]].getValue() : cells_[n_[1]].getValue();

        }

        int getNumberNeighborsInDim
        (const int idx, std::vector<int> &m, const int dim)   {
            n_neighs = 0;
            getNeighborsInDim(idx, n_, dim);
            m = n_;
            return n_neighs;
        }

        /**
         * Computes the indices of the 4-connectivity neighbors. As it is based
         * on vectors (to improve performance) the number of neighbors found is
         * returned since the neighs vector will have always the same size.
         *
         * @param idx index of the cell the neighbors are desired.
         * @param neighs constains the neigbour indices.
         *
         * @return the number of neighbors found.
         * */
        int getNeighbors
        (const int idx, std::vector<int> & neighs) {
            n_neighs = 0;
            for (int i = 0; i < ndims_; ++i)
                getNeighborsInDim(idx, neighs, i);

            return n_neighs;
        }

        /**
         * Computes the indices of the 4-connectivity neighbors in a specified direction.
         * This function is designed to be used within getNeighbors() or getMinValueInDim()
         * since it increments the private member n_neighs and it is only reset in
         * those functions.
         *
         * @param idx index of the cell the neighbors are desired.
         * @param neighs constains the neigbour indices.
         * @param dim the direction (dimension) to be examined.
         *
         * @see getNeighbors()
         * @see getMinValueInDim()
         * */
        void getNeighborsInDim
        (const int idx, std::vector<int>& neighs, const int dim) {
            int c1,c2;
            if (dim == 0) {
                c1 = idx-1;
                c2 = idx+1;
                // Checking neighbor 1.
                if ((c1 >= 0) && (c1/d_[0] == idx/d_[0]))
                    neighs[n_neighs++] = c1;
                // Checking neighbor 2.
                //if ((c2 < ncells_) && (c2/d_[0] == idx/d_[0])) // full check, not necessary.
                if (c2/d_[0] == idx/d_[0])
                    neighs[n_neighs++] = c2;
            }
            else {
                // neighbors proposed.
                c1 = idx-d_[dim-1];
                c2 = idx+d_[dim-1];
                // Checking neighbor 1.
                if ((c1 >= 0) && (c1/d_[dim] == idx/d_[dim]))
                    neighs[n_neighs++] = c1;
                // Checking neighbor 2.
                //if ((c2 < ncells_) && (c2/d_[dimi] == idx/d_[dim])) // full check, not necessary.
                if (c2/d_[dim] == idx/d_[dim])
                    neighs[n_neighs++] = c2;
            }
        }

        /**
         * Transforms from index coordinates to coordinates.
         *
         * @param idx the index to be transformed
         * @param coords outputs the coordinate for each dimension.
         *
         * @return -1 if there were any problem. 1 if successful.
         *
         * @see showCoords()
         * @see showIdx()
         * @see coord2idx()
         * */
        int idx2coord
        (const int idx, std::vector<int>& coords) {
            if (coords.size() != ndims_)
                return -1;
            else {
                coords[ndims_-1] = idx/d_[ndims_-2]; // First step done apart.
                int aux = idx - coords[ndims_-1]*d_[ndims_-2];
                for (int i = ndims_ - 2; i > 0; --i) {
                    coords[i] = aux/d_[i-1];
                    aux -= coords[i]*d_[i-1];
                }
                coords[0] = aux; //Last step done apart.
            }
            return 1;
        }

        /**
         * Transforms from coordinates to index.
         *
         * @param coords contains the coordinate for each dimension.
         * @param idx outputs the corresponding index in the flat vector.
         *
         * @return -1 if there were any problem. 1 if successful.
         *
         * @see showCoords()
         * @see showIdx()
         * @see idx2coord()
         * */
        int coord2idx
        (const std::vector<int> & coords, int & idx) {
            if (coords.size() != ndims_)
                return -1;
            else {
                idx = coords[0];
                for(int i = 1; i < ndims_; ++i)
                    idx += coords[i]*d_[i-1];
            }
            return 1;
        }

       /**
         * Shows the coordinates from an index.
         *
         * @param idx the index to be transformed into coordinated and shown.
         *
         * @see showIdx()
         * @see idx2coord()
         * @see coord2idx()
         * */
        void showCoords
        (const int idx) {
            std::vector<int> coords (ndims_,0);
            idx2coord(idx, coords);
            for (int i = 0; i < ndims_; ++i)
                std::cout << coords[i] << "\t";
            std::cout << std::endl;
        }

        /**
         * Shows the index from the coordinates
         *
         * @param coords the coordinate for each dimension and to be transformed into index and shown.
         *
         * @see showCoords()
         * @see idx2coord()
         * @see coord2idx()
         * */
        void showIdx
        (const std::vector<int>& coords) {
            int idx;
            coord2idx(coords, idx);
            std::cout << idx << '\n';
        }

        int size
        () const {
            return ncells_;
        }

        /**
        @return the maximum value of the cells within the grid.
         * */
        double getMaxValue
        () const {
            double max = 0;
            for (T & c:cells_) {
                if (!isinf(c.getValue()) && c.getValue() > max)
                    max = c.getValue();
            }
            return max;
        }

        bool isClear
        () const {
            return clear_;
        }

        void setClear
        (const bool c) {
            clear_ = c;
        }

        void clear
        () {
            for (T & c:cells_)
                c.setDefault();
            clear_ = true;
        }

        /** Makes the number of dimensions of the grid available at compilation time.
        @return number of dimensions of the grid.
         * */
        const size_t getNDims() const {return ndims_;}

    protected:
        std::vector<T> cells_;  /*!< The main container for the class. */
        std::vector<int> dimsize_;  /*!< Contains the size of each dimension. */
        float leafsize_;  /*!< Real size of the cells. It is assumed that the cells in the grid are cubic. */
        int ncells_;  /*!< Number of cells in the grid (size) */
        bool clear_;  /*!< Flag to indicate if the grid is ready to use. */
        size_t ndims_;

    private:
        // Auxiliar vectors to speed things up.
        std::vector<int> d_;  /*!< Auxiliar vector to speed up neighbor and indexing generalization: stores parcial multiplications of dimensions sizes. d_[0] = dimsize_[0];
                                                                                                             d_[1] = dimsize_[0]*dimsize_[1]; etc.*/
        std::vector<int> n_; /*!< Auxiliar vector to speed up neighbor and indexing generalization: for getMinValueInDim() function.*/
        int n_neighs; /*!<  Internal variable that counts the number of neighbors found in every iteration. Modified by getNeighbours(), getNeighborsInDim() and getMinValueInDim(). functions.*/
};

#endif /* NDGRIDCELL_H_*/
