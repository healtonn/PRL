/*
* Pruchod preorder, Jakub Jochlík, xjochl00
*/
#include <iostream>
#include <vector>
#include <math.h>
#include <mpi.h>

#define INDEX_CORRECTION 1          //to help address next edge
#define ROOT 0
#define EDGE_MULTIPLIER 4           // since there are two oriented edges per binary tree edge, the edge index step is 4
#define FATHER_NODE_EDGE_MULTIPLIER 2           // same priciple
#define NO_PREVIOUS_EDGE -1


using namespace std;


class Edge{
    public:
        int edgeID;
        int reverseEdgeID;
        int previousEdgeID;
        int nextEdgeID;
        int sourceNode;
        int destinationNode;
        int amountOfEdges;
        int etourPart;
        int weight;
        int weightPart;
        bool lastEdge;
        MPI_Status edgeMpiStatus;

        vector<int> adjescencyList;


        Edge(int id, int edgeLimit){
            edgeID = id;
            amountOfEdges = edgeLimit;
            
        }

        void initEdge(){
            if(edgeID % 2 == 0){   //go from A to B
                sourceNode = edgeID / 4;
                destinationNode = edgeID / 2 + 1;
                reverseEdgeID = edgeID + 1;
                weight = 1;
            }else{                  //go from B to A - reversed edge
                sourceNode = edgeID / 2 + 1;
                destinationNode = edgeID / 4;
                reverseEdgeID = edgeID - 1;
                weight = 0;         //ignore reversed edge - preorder
            }
        }

        void makeAdjescencyList(int numberOfNodes){
            if(destinationNode != ROOT){
                adjescencyList.push_back((destinationNode - INDEX_CORRECTION) * FATHER_NODE_EDGE_MULTIPLIER + 1);   //OUT edge
                adjescencyList.push_back((destinationNode - INDEX_CORRECTION) * FATHER_NODE_EDGE_MULTIPLIER);       //IN edge
            }
            if((destinationNode + INDEX_CORRECTION) * 2 <= numberOfNodes){     //can i go left?
                adjescencyList.push_back(destinationNode * EDGE_MULTIPLIER);         //OUT edge
                adjescencyList.push_back(destinationNode * EDGE_MULTIPLIER + 1);     //IN edge
            }
            if((destinationNode + INDEX_CORRECTION) * 2 + 1 <= numberOfNodes){      //can i go right?
                adjescencyList.push_back(destinationNode * EDGE_MULTIPLIER + 2);         //OUT edge
                adjescencyList.push_back(destinationNode * EDGE_MULTIPLIER + 3);     //IN edge
            }
        }

        void findCorrectEtourPart(){
            for(int i = 0; i < adjescencyList.size(); i += 2){
                if(adjescencyList[i] == reverseEdgeID){
                    if(i + 2 < adjescencyList.size())      //if next exists
                        etourPart = adjescencyList[i + 2];
                    else                //next is null, get first
                        etourPart = adjescencyList[0];
                    break;
                }
            }
        }

        void findPrevousEdge(int etour[], int sizeOfArray){
            if(edgeID == 0){
                previousEdgeID = NO_PREVIOUS_EDGE;
                return;
            }

            for(int i = 0; i < sizeOfArray; i++){
                if(etour[i] == edgeID){
                    previousEdgeID = i;
                    break;
                }
            }
        }

        void findNextEdge(int etour[]){
                nextEdgeID = etour[edgeID] == 0 ? -1 : etour[edgeID];
        }

        void calculateFinalWeight(int numberOfEdges){
            int lastEdgeIndex = numberOfEdges - INDEX_CORRECTION;
            int weightFromSender;
                if(nextEdgeID == -1){
                    weightPart = weight;
                    MPI_Send(&weightPart, 1, MPI_INT, previousEdgeID, 0, MPI_COMM_WORLD);
                }else if(previousEdgeID == -1){
                    MPI_Recv(&weightFromSender, 1, MPI_INT, etourPart, 0, MPI_COMM_WORLD, &edgeMpiStatus);
                    weightPart = weightFromSender + weight;
                }else{
                    MPI_Recv(&weightFromSender, 1, MPI_INT, etourPart, 0, MPI_COMM_WORLD, &edgeMpiStatus);
                    weightPart = weightFromSender + weight;
                    MPI_Send(&weightPart, 1, MPI_INT, previousEdgeID, 0, MPI_COMM_WORLD);
                }
            weightPart = weight == 0 ? -1 : weightPart;
            MPI_Barrier(MPI_COMM_WORLD);
        }


        // DEBUG PRINTS
        void printEdge(){
            cout << "I am edge " << edgeID << " and i am going from node " << sourceNode << " to node " << destinationNode << endl;
        }

        void printAdjescencyList(){
            cout << "I am edge " << edgeID << " with neighbours:";
            for(int i = 0; i < adjescencyList.size(); i += 2){
                cout  << " " << adjescencyList[i] << " " << adjescencyList[i + 1];
            }
            cout << endl;
        }
};

int getHighestNumberInArray(int array[], int arraySize){
    int highestNumber = -1;
    for(int i = 0; i < arraySize; i++){
        if(highestNumber < array[i])
            highestNumber = array[i];
    }

    return highestNumber;
}

int main(int argc, char *argv[]){
    int numberOfProcessors;         //one processor per oriented edge in graph/tree
    int processorID;
    MPI_Status mpiStatus;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numberOfProcessors);
    MPI_Comm_rank(MPI_COMM_WORLD, &processorID);

    //Inicialization, checks, stuff
    if(argc != 2){
        cout << "wrong parameter usage" << endl;
        return 1;
    }

    string inputTree = argv[1];
    if(inputTree.length() == 1){
        if(processorID == 0)
            cout << inputTree << endl;
        MPI_Finalize();
        return 0;
    }

    Edge edge(processorID, inputTree.length());
    edge.initEdge();
    //edge.printEdge();
    edge.makeAdjescencyList(inputTree.length());
    //edge.printAdjescencyList();
    edge.findCorrectEtourPart();

    int etour[numberOfProcessors];
    etour[processorID] = edge.etourPart;
    MPI_Allgather(&etour[processorID], 1, MPI_INT, etour, 1, MPI_INT, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    edge.findPrevousEdge(etour, sizeof(etour)/sizeof(*etour));
    edge.findNextEdge(etour);
    edge.calculateFinalWeight(numberOfProcessors);
    MPI_Barrier(MPI_COMM_WORLD);

    if(processorID == 0){ //gather results and build prefix array
        int finalWeigths[numberOfProcessors];
        int destinations[numberOfProcessors];
        int recievedWeight;
        int recievedsource;
        finalWeigths[processorID] = edge.weightPart;
        destinations[0] = edge.destinationNode;
        for(int i = 1; i < numberOfProcessors; i++){        //request all data
            MPI_Recv(&recievedWeight, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &mpiStatus);
            MPI_Recv(&recievedsource, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &mpiStatus);
            finalWeigths[i] = recievedWeight;
            destinations[i] = recievedsource;
        }

        int highestNumber = getHighestNumberInArray(finalWeigths, numberOfProcessors);
        int preorderSize = highestNumber + 1;
        int preorder[preorderSize];
        int index = 0;
        preorder[index] = 0;
        index++;
        while(true){            //select correct preorder elements and save them
            for(int i = 0; i < numberOfProcessors; i++){
                if(highestNumber == finalWeigths[i]){
                    preorder[index] = destinations[i];
                    index++;
                    highestNumber--;
                    break;
                }
            }

            if(highestNumber <= 0)
                break;
        }

        for(int i = 0; i < preorderSize; i++){      //print result
            cout << inputTree[preorder[i]];
        }
        cout << endl;


    }else{      //send results to main guy
        for(int i = 1; i < numberOfProcessors; i++){
            MPI_Send(&edge.weightPart, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&edge.destinationNode, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }

    //edge.edgeID = processorID;



    MPI_Finalize();
    return 0;
}