    #ifndef MATRIX_H    
    #define MATRIX_H  
    #include<iostream>
    #include<vector>
    using namespace std;


    class Matrix {
    private:
        double** data;
        

    public:
        int rows, cols;
        Matrix(int r, int c) : rows(r), cols(c) {
            data = new double*[rows];          
            for (int i = 0; i < rows; ++i) {
                data[i] = new double[cols];    
            }
        }
        Matrix(const Matrix& other) : rows(other.rows), cols(other.cols) {
            data = new double*[rows];
            for (int i = 0; i < rows; ++i) {
                data[i] = new double[cols];
                for (int j = 0; j < cols; ++j) {
                    data[i][j] = other.data[i][j];
                }
            }
        }

        void set(int row, int col, double value){
            if(row >= rows || col >= cols || row < 0 || col < 0){
                throw invalid_argument("matrix mismatch");
            }
            data[row][col] = value;
        }

        double get(int row, int col) const {
            if(row >= rows || col >= cols || row < 0 || col < 0){
                throw invalid_argument("matrix mismatch");
            }
            return data[row][col];
        }
        
        Matrix operator*(const Matrix &other) const {
            if (this->cols != other.rows) {
            throw invalid_argument("Matrix mismatch!");}
            
            Matrix result(this->rows, other.cols);

            for (int i = 0; i < rows; i++) { 
                for (int j = 0; j < other.cols; j++) { 
                double sum = 0;
                    for (int k = 0; k < cols; ++k) { 
                        sum += this->data[i][k] * other.data[k][j];
                    }
                result.set(i,j,sum);
            }
            }
            return result;
        }

        Matrix operator+(const Matrix &other) const {
            if(this->cols != other.cols || this->rows != other.rows){
                throw invalid_argument("mismatch matrix!");
            }

            Matrix result(this->rows, this->cols);

            for(int row = 0; row < rows; row++){
                for(int col = 0; col < cols; col++){
                    result.set(row, col, this->data[row][col] + other.data[row][col]);
                }
            }
            return result;
        }

        double& operator()(int r, int c) {
            return data[r][c];
        }

        Matrix& operator=(const Matrix& other) {
            if (this == &other) return *this;
            
            for (int i = 0; i < rows; ++i) {
                delete[] data[i];
            }
            delete[] data;
            
            rows = other.rows;
            cols = other.cols;
            data = new double*[rows];
            for (int i = 0; i < rows; ++i) {
                data[i] = new double[cols];
                for (int j = 0; j < cols; ++j) {
                    data[i][j] = other.data[i][j];
                }
            }
            return *this;
        }

        ~Matrix() {
            for (int i = 0; i < rows; ++i) {
                delete[] data[i];           
            }
            delete[] data;                  
        }
    };


    #endif             