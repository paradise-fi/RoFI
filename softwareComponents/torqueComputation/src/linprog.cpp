#include <torqueComputation/linprog.hpp>

using namespace rofi::torqueComputation;
using namespace arma;

namespace rofi::torqueComputation {
    void argsSanityCheck(
            const mat& constraintMatrix,
            const vec& constraintLowerBounds,
            const vec& constraintUpperBounds,
            const vec& lowerBounds,
            const vec& upperBounds,
            const std::optional<vec>& objectiveCoefficients
    ) {
        arma::uword numberRows = constraintMatrix.n_rows;
        arma::uword numberColumns = constraintMatrix.n_cols;

        if (constraintLowerBounds.n_rows != numberRows) {
            throw std::logic_error("Constraint lower bound length should equal to number of rows of constraint matrix.");
        }
        if (constraintUpperBounds.n_rows != numberRows) {
            throw std::logic_error("Constraint upper bound length should equal to number of rows of constraint matrix.");
        }
        if (lowerBounds.n_rows != numberColumns) {
            throw std::logic_error("Lower bounds length should equal to number of columns of constraint matrix.");
        }
        if (upperBounds.n_rows != numberColumns) {
            throw std::logic_error("Upper bounds length should equal to number of columns of constraint matrix.");
        }
        if (objectiveCoefficients != std::nullopt && objectiveCoefficients.value().n_rows != numberColumns) {
            throw std::logic_error("Objective function coefficients length should equal to number of columns of constraint matrix.");
        }
    }

    void copyVecToArray(const vec& colVector, uword size, double * location) {
        memcpy(location, colVector.begin(), size * sizeof(double));
    }

    OptimizeResult linprog(
            const mat& constraintMatrix,
            const vec& constraintLowerBounds,
            const vec& constraintUpperBounds,
            const vec& lowerBounds,
            const vec& upperBounds,
            const std::optional<vec>& objectiveCoefficients /*= std::nullopt*/
    ) {
        argsSanityCheck(
                constraintMatrix,
                constraintLowerBounds,
                constraintUpperBounds,
                lowerBounds,
                upperBounds,
                objectiveCoefficients
        );

        // Empty model
        ClpSimplex  model;

        // CLP support only ints for size of matrix.
        int numberRows = constraintMatrix.n_rows & INT_MAX;;
        int numberColumns = constraintMatrix.n_cols & INT_MAX;
        int numberElements = numberRows * numberColumns & INT_MAX;
        // Arrays will be set to default values
        model.resize(numberRows, numberColumns);

        // CLP library CoinPackedMatrix destructor takes care of deleting this.
        double * elements = new double [numberElements];

        // CLP library CoinPackedMatrix destructor takes care of deleting this.
        CoinBigIndex * starts = new CoinBigIndex [numberColumns+1];

        // CLP library CoinPackedMatrix destructor takes care of deleting this.
        int * rows = new int [numberElements];
        std::iota(rows, rows + numberRows, 0);
        for (int i = 1; i < numberColumns; i++) {
            memcpy(rows + i * numberRows, rows, numberRows * sizeof(int));
        }

        // CLP library CoinPackedMatrix destructor takes care of deleting this.
        int * lengths = new int[numberColumns];
        std::fill_n(lengths, numberColumns, numberRows);

        double * columnLower = model.columnLower();
        double * columnUpper = model.columnUpper();
        double * objective = model.objective();
        double * rowLower = model.rowLower();
        double * rowUpper = model.rowUpper();

        //Set objective
        if (objectiveCoefficients != std::nullopt) {
            copyVecToArray(objectiveCoefficients.value(), numberColumns, objective);
        }

        // Set x bounds
        copyVecToArray(lowerBounds, numberColumns, columnLower);
        copyVecToArray(upperBounds, numberColumns, columnUpper);

        // Set rows bounds
        copyVecToArray(constraintLowerBounds, numberRows, rowLower);
        copyVecToArray(constraintUpperBounds, numberRows, rowUpper);

        // Set constraints matrix
        for (int j = 0; j < numberColumns; j++) {
            starts[j] = j * numberRows;
            copyVecToArray(constraintMatrix.col(j), numberRows, elements + j * numberRows);
        }

        starts[numberColumns] = numberElements;

        // assign to matrix - is deleted in ClpPackedMatrix destructor.
        CoinPackedMatrix * matrix = new CoinPackedMatrix(true, 0.0, 0.0);
        matrix->assignMatrix(true, numberRows, numberColumns, numberElements,
                             elements, rows, starts, lengths);

        // Deleted in ClpSimplex (ClpModel) destructor.
        ClpPackedMatrix * clpMatrix = new ClpPackedMatrix(matrix);
        model.replaceMatrix(clpMatrix, true);

        int options = 256 + (objectiveCoefficients == std::nullopt ? 128 : 0);
        model.setSpecialOptions(options);

        model.setLogLevel(0);

        int feasible = model.primal();
        double *result = model.primalColumnSolution();
        auto resultVec = std::vector<double>(result, result + numberColumns);

        return OptimizeResult {
                feasible == 0,
                model.rawObjectiveValue(),
                resultVec
        };
    }
}
