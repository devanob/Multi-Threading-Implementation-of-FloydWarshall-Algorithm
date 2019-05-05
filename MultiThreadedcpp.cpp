#include <thread>
#include <iostream>
#include <limits>
#include <vector>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <omp.h>
//This Will Serve As Our Infinite
//and will be used to check the values in the matrix whether to be replaced or not
static int INF = std::numeric_limits < int > ::max();

//Forward Declare Class
class semaphore {
private:
    std::mutex mutex_lock;
    std::condition_variable condition_var;
    unsigned long count_ = 1; // Initialized as locked.
public:
    semaphore(const unsigned long & count);
    void signal();
    void wait();
};

//Function delarations
void floydWarshallMultiThreadedSub(std::vector < std::vector < int >> & matrix,
    int& readCount, std::mutex& readCountLock, semaphore &matrixLock,
    int k, int i);

void floydWarshall(std::vector < std::vector < int >> & matrix);

void floydWarshallMultiThreaded(std::vector < std::vector < int >> & matrix);

void printMatrix(const std::vector < std::vector < int >> & distanceMatrix);

std::vector < std::vector < int >> contructMatrix();
//user define semaphore class

int main(int argc, char ** argv) {


    //std::cout << listArg[0];
    std::vector < std::vector < int >> graphMatrix = std::move(contructMatrix());
     std::vector < std::vector < int >> distMatrix = graphMatrix; //makea copy of the grpah matrix
    //std::cout << distanceMatrix.size();
    //printMatrix(distanceMatrix);
    //auto start = std::chrono::high_resolution_clock::now();
    floydWarshallMultiThreaded(distMatrix);
   // auto finish = std::chrono::high_resolution_clock::now();
    printMatrix(distMatrix);
    //Time For multi-threading
    //user to control the read count variable
    return 1;
}

void floydWarshall(std::vector < std::vector < int >> & matrix) {

    for (int k = 0; k < matrix.size(); k++)
//#pragma omp parallel
//#pragma omp for
        for (int i = 0; i < matrix.size(); i++)
            for (int j = 0; j < matrix.size(); j++) {
                if (matrix[i][k] != INF &&
                    matrix[k][j] != INF &&
                    matrix[i][k] + matrix[k][j] < matrix[i][j])
                    matrix[i][j] = matrix[i][k] + matrix[k][j];

            }
}

void floydWarshallMultiThreadedSub(std::vector < std::vector < int >> & matrix,
    int& readCount, std::mutex& readCountLock, semaphore &matrixLock, int k, int i) {
    //std::cout << "Thread :" << i <<" k = " << k  << " enter Sub" << std::endl;
    //std::cout <<"Read Count " << readCount << std::endl;
    //std::cout.flush();
    bool needsWrite;
    for (int j = 0; j < matrix.size(); j++) {
        //Read Read Count-Critical Area
        readCountLock.lock(); //lock read or write to ThreadCount
        readCount++;
        if (readCount == 1) {
            matrixLock.wait(); // signal that no reader are allow if one thread is inside
        }
        readCountLock.unlock();//unlock Thread Count
        needsWrite = (matrix[i][k] != INF && matrix[k][j] != INF &&
            matrix[i][k] + matrix[k][j] < matrix[i][j]);

        readCountLock.lock(); //lock read or write to ThreadCount
        readCount--;
        if (readCount == 0) {
            matrixLock.signal(); //no threads are inside
        }
        readCountLock.unlock();

        if (needsWrite) {//If the thread needs to write then
            matrixLock.wait(); //Waits to get into the matrix
            matrix[i][j] = matrix[i][k] + matrix[k][j];
            matrixLock.signal(); //signal other writer/reader the read the matrix is avaible

        }
        //if stays reader



      //End Read Cound


    }
}



void floydWarshallMultiThreaded(std::vector < std::vector < int >> & matrix) {
    //std::cout << "MutltiThreaded";
    int readCount = 0;
    std::mutex readCountLock;
    semaphore matrixLock(0);
    std::vector<std::thread> threadsIloop(matrix.size());
    //std::cout << threadsIloop.size();// create thread contain for each i iteration
    for (int k = 0; k < matrix.size(); k++) {
        for (int i = 0; i < matrix.size(); i++) {
            threadsIloop[i] = std::move(std::thread(floydWarshallMultiThreadedSub,
                std::ref(matrix),
                std::ref(readCount),
                std::ref(readCountLock),
                std::ref(matrixLock),
                k, i));

        }
        for (int threadi = 0; threadi < threadsIloop.size(); threadi++) {
            threadsIloop[threadi].join();
        }
        readCount = 0;
    }


}

std::vector < std::vector < int >> contructMatrix() {
    std::cout << "Enter Your Matrix Information in The Format <NodeCount> <VerticesCount> <Vertices> :" << std::endl;
    std::cout << "----->(no Brackects<>):";
    int nodeCount; // get the nodeCount ;
    int edgeCount;  //The the amount of edges
    std::cin >> nodeCount;
    if (nodeCount < 0 ){
        throw std::runtime_error("You Must Enter Node Count Greater Than Zero : ");
    }
    std::cin >> edgeCount;
    if (edgeCount < 0 ){
        throw std::runtime_error("You Must Enter edgeCount Count Greater Than Zero");
    }
    std::vector < std::vector < int >> graph(nodeCount); //The matrix
    graph.resize(nodeCount);
    //Create nNode*nNode Matrix
    //------------------------------------//
    //Create Intialize Every ELement To INF : Check Top
    for (unsigned int i = 0; i < nodeCount; i++) {
        graph[i] = std::move(std::vector < int >(nodeCount, INF));
    }
    //------------------------------------//
    //Make Diagonal 0 ->[i][i]
    for (unsigned int i = 0; i < graph.size(); i++) {
        graph[i][i] = 0;
    } //ptr to teh vector for the ui vi weight
    int i; //the jth vertic2
    int j; //the kth vertice
    int w; //the weight
    for (int i = 0; i <= edgeCount; i++) {

        std::cin >> i; //get the ith node
        std::cin >> j;  //get the jth node  i->j = w
        std::cin >> w; //i->j = w
        graph[i - 1][j - 1] = w; //i->j = w
        graph[j - 1][i - 1] = w;
    }

    return graph;

}

//Semaphore overloading contructor to override the initial value of the lock
semaphore::semaphore(const unsigned long & count) {
    this->count_ = count;
}

//
void semaphore::signal() {
    std::lock_guard < decltype(mutex_lock) > lock(mutex_lock);
    ++count_;
    condition_var.notify_one();
}

void semaphore::wait() {
    std::unique_lock < decltype(mutex_lock) > lock(mutex_lock);
    condition_var.wait(lock, [this] {
        return !count_;
    });
    --count_;
}

//This function is used to print the matix
void printMatrix(const std::vector < std::vector < int >> & distanceMatrix) {
    for (int i = 0; i < distanceMatrix.size(); i++) { //taking the i vals to construct the rows
        for (int j = 0; j < distanceMatrix.size(); j++) { //taking the j vals to construct the columns
            if (distanceMatrix[i][j] == INF) {
                std::cout << "INF" << " "; //If infinite then print out that value
            }
            else {
                std::cout << distanceMatrix[i][j] << "   "; //If not infinite then print the updated value or not the updated one
            }
        }
        std::cout << std::endl;
    }

}
