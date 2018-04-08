/*
* Merge-splitting sort, Jakub Jochlík, xjochl00
*/

#include <iostream>
#include <fstream>
#include <mpi.h>
#include <vector>
#include <algorithm>

#define MESSAGE_TAG 0
#define MESSAGE_INFO 1
#define MESSAGE_INT 2
#define MESSAGE_GET_RESULT 3
#define FILLER 999999

using namespace std;

void printIntVector(vector<int> vector);

int main(int argc, char *argv[]){
    int numberOfProcessors;
    int processorID;
    int amountOfNumbers;
    int missingNumbers = 0;
    int numbersPerProcessor;
    int numberToSend;   // send n/p numbers to each processor, skip missing for the last one
    int receivedNumber; //recieve n/p numbers per processor
    vector<int> numbers;
    vector<int> processorNumbers;
    MPI_Status mpiStatus;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numberOfProcessors);
    MPI_Comm_rank(MPI_COMM_WORLD, &processorID);

    if (processorID == 0) {     //load numbers from file and store them
        fstream file;
        int number;
        file.open("numbers", ios::in);
        while(file.good()){
            number = file.get();
            if (!file.good()) break;
            numbers.push_back(number);
        }
        file.close();

        for (int i = 0; i != numbers.size(); i++){
            cout << numbers[i] << " ";
        }
        cout << endl;

        // delaing with undivadable number of numbers and procesors
        amountOfNumbers = numbers.size();
        missingNumbers = numberOfProcessors - amountOfNumbers % numberOfProcessors;
        numbersPerProcessor = amountOfNumbers / numberOfProcessors;
        if(missingNumbers == numberOfProcessors)missingNumbers = 0;
        else numbersPerProcessor++;
        if(missingNumbers >= numbersPerProcessor) { //fix for too big difference in processors and numbers
            for(int i = missingNumbers; i >= numbersPerProcessor; i--)
                numbers.push_back(FILLER);
        }

        for (int sendToProcessor = 0; sendToProcessor < numberOfProcessors; sendToProcessor++){
            MPI_Send(&amountOfNumbers, 1, MPI_INT, sendToProcessor, MESSAGE_INFO, MPI_COMM_WORLD);
            MPI_Send(&missingNumbers, 1, MPI_INT, sendToProcessor, MESSAGE_INFO, MPI_COMM_WORLD);
            MPI_Send(&numbersPerProcessor, 1, MPI_INT, sendToProcessor, MESSAGE_INFO, MPI_COMM_WORLD);
            for(int j = 0; j < numbersPerProcessor; j++){
                if (numbers.size() == 0) break;
            numberToSend = numbers.front();
            numbers.erase(numbers.begin());
            MPI_Send(&numberToSend, 1, MPI_INT, sendToProcessor, MESSAGE_TAG, MPI_COMM_WORLD);
            }
        }
    }

    //some values that all need for sorting
    MPI_Recv(&amountOfNumbers, 1, MPI_INT, 0, MESSAGE_INFO, MPI_COMM_WORLD, &mpiStatus);
    MPI_Recv(&missingNumbers, 1, MPI_INT, 0, MESSAGE_INFO, MPI_COMM_WORLD, &mpiStatus);
    MPI_Recv(&numbersPerProcessor, 1, MPI_INT, 0, MESSAGE_INFO, MPI_COMM_WORLD, &mpiStatus);
    int iterationSteps = numberOfProcessors / 2;
    bool tmp = true;    //for helping handle missing numbers

    for(int j = 0; j < numbersPerProcessor; j++){
        MPI_Recv(&receivedNumber, 1, MPI_INT, 0, MESSAGE_TAG, MPI_COMM_WORLD, &mpiStatus);
        processorNumbers.push_back(receivedNumber);
        if ((processorID == (numberOfProcessors - 1)) && tmp){
            j += missingNumbers;
            tmp = false;
        } 
    }
    
    sort(processorNumbers.begin(), processorNumbers.end());     //sort your own numbers before connecting with your neighbour

    for(int step = 0; step <= iterationSteps + 2; step++){      // + 2 for preparation steps
        if (processorID >= numberOfProcessors) break;
        if((step % 2) == 1){              //step 1
            if((processorID % 2) == 0){     // in step 1 only processors with id 0,2,4,.. are sending first
                if(processorID != (numberOfProcessors - 1)){          //if i am not the most right one i slack
                    for (int x = 0; x < numbersPerProcessor; x++){      //send my numbers to right neighbour
                        numberToSend = processorNumbers.front();
                        processorNumbers.erase(processorNumbers.begin());
                        MPI_Send(&numberToSend, 1, MPI_INT, processorID + 1, MESSAGE_INT, MPI_COMM_WORLD);
                    }
                    
                    for (int x = 0; x < numbersPerProcessor; x++){      //send my numbers to right neighbour
                        MPI_Recv(&receivedNumber, 1, MPI_INT, processorID + 1, MESSAGE_INT, MPI_COMM_WORLD, &mpiStatus);
                        processorNumbers.push_back(receivedNumber);
                    }

                    //
                } //if(processorID != numberOfProcessors - 1)
            }//if((processorID % 2) == 0)
            else{       //processors with id 1,3,5,..
                if((processorID % 2) == 1){            //i have something to recieve
                    for (int x = 0; x < numbersPerProcessor; x++){      //send my numbers to right neighbour
                        MPI_Recv(&receivedNumber, 1, MPI_INT, processorID - 1, MESSAGE_INT, MPI_COMM_WORLD, &mpiStatus);
                        processorNumbers.push_back(receivedNumber);
                    }

                    sort(processorNumbers.begin(), processorNumbers.end());
                    for (int x = 0; x < numbersPerProcessor; x++){      //send my numbers to right neighbour
                        numberToSend = processorNumbers.front();
                        processorNumbers.erase(processorNumbers.begin());
                        MPI_Send(&numberToSend, 1, MPI_INT, processorID - 1, MESSAGE_INT, MPI_COMM_WORLD);
                    }
                } // if(((processorID % 2) == 0) && (processorID != numberOfProcessors - 1))
            }
        } // if((step % 2) == 1)
        else{                           //step 2
            if((processorID % 2) == 0){ //during step 2 proc 2,4,.. recieve first
                if(processorID != 0){       //processor 0 does nothing this step
                    for (int x = 0; x < numbersPerProcessor; x++){      //send my numbers to right neighbour
                        MPI_Recv(&receivedNumber, 1, MPI_INT, processorID - 1, MESSAGE_INT, MPI_COMM_WORLD, &mpiStatus);
                        processorNumbers.push_back(receivedNumber);
                    }

                    sort(processorNumbers.begin(), processorNumbers.end());
                    for (int x = 0; x < numbersPerProcessor; x++){      //send my numbers to right neighbour
                        numberToSend = processorNumbers.front();
                        processorNumbers.erase(processorNumbers.begin());
                        MPI_Send(&numberToSend, 1, MPI_INT, processorID - 1, MESSAGE_INT, MPI_COMM_WORLD);
                    }
                }
            }
            else{       //during step 2 procesors 1,3,5,... send first
                if(processorID != (numberOfProcessors -1)){          //i am not the most righ one
                    for (int x = 0; x < numbersPerProcessor; x++){      //send my numbers to right neighbour
                        numberToSend = processorNumbers.front();
                        processorNumbers.erase(processorNumbers.begin());
                        MPI_Send(&numberToSend, 1, MPI_INT, processorID + 1, MESSAGE_INT, MPI_COMM_WORLD);
                    }

                    for (int x = 0; x < numbersPerProcessor; x++){      //send my numbers to right neighbour
                        MPI_Recv(&receivedNumber, 1, MPI_INT, processorID + 1, MESSAGE_INT, MPI_COMM_WORLD, &mpiStatus);
                        processorNumbers.push_back(receivedNumber);
                    }
                }   //if(processorID != numberOfProcessors -1)
            }
        }
    }   //for(int step = 1; step <= iterationSteps; step++)

    tmp = true;
    if (processorID == 0){      //request sorted values
        int sendMeYourResult = -10;
        for (int i = 1; i < numberOfProcessors; i++){           //synchronize requests, first get all from one then next one,....
            for(int x = 0; x < numbersPerProcessor; x++){
                if(i == numberOfProcessors - 1 && tmp) {x += missingNumbers;tmp = false;}
                MPI_Send(&sendMeYourResult, 1, MPI_INT, i, MESSAGE_GET_RESULT, MPI_COMM_WORLD);
                MPI_Recv(&receivedNumber, 1, MPI_INT, i, MESSAGE_GET_RESULT, MPI_COMM_WORLD, &mpiStatus);
                if (receivedNumber!= FILLER)
                    processorNumbers.push_back(receivedNumber);
            }
        }
        printIntVector(processorNumbers);       // show final sorted numbers
    }
    else{   //send back results as request are recieved
        for(int x = 0; x < numbersPerProcessor; x++){
                if(processorID == numberOfProcessors - 1 && tmp) {x += missingNumbers;tmp = false;}
                MPI_Recv(&receivedNumber, 1, MPI_INT, 0, MESSAGE_GET_RESULT, MPI_COMM_WORLD, &mpiStatus);
                numberToSend = processorNumbers.front();
                processorNumbers.erase(processorNumbers.begin());
                MPI_Send(&numberToSend, 1, MPI_INT, 0, MESSAGE_GET_RESULT, MPI_COMM_WORLD);
            }
    }

    MPI_Finalize();
    return 0;
}

/*
* print integers from vector
*/
void printIntVector(vector<int> vector){
    for (int i = 0; i != vector.size(); i++){
        cout << vector[i] << endl;
    }
}
