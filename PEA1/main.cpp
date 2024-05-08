#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <windows.h>

using namespace std;

int** edges; //macierz sąsiedztwa
int nodeCount; //liczba wierzchołków

double PCFreq = 0.0;
__int64 CounterStart = 0;

void startCounter() {
    LARGE_INTEGER li;
    if( !QueryPerformanceFrequency( & li ) )
        cout << "QueryPerformanceFrequency failed!\n";

    PCFreq = double( li.QuadPart ) / 1000.0;

    QueryPerformanceCounter( & li );
    CounterStart = li.QuadPart;
}
double getCounter() {
    LARGE_INTEGER li;
    QueryPerformanceCounter( & li );
    return double( li.QuadPart - CounterStart ) / PCFreq;
}

void readFile(const string& fileName){
    ifstream file(fileName);
    file >> nodeCount;
    edges = new int*[nodeCount];
    for (int i = 0; i < nodeCount; i++){
        edges[i] = new int[nodeCount];
    }
    for (int i = 0; i < nodeCount; i++){
        for (int j = 0; j < nodeCount; j++){
            string s;
            file >> s;
            int number;
            number = stoi(s);
            edges[i][j] = number;
        }
    }
    file.close();
}

void bruteforce(ofstream& saveFile, int a){
    vector <int> nodes; //wektor zawierający kolejne numery wierzchołków
    //inicjalizacja wektora - po wykonaniu algorytmu nie nadaje sie do uzytku
    for (int i = 1; i < nodeCount; i++){
        nodes.push_back(i);
    }
    vector <int> savedSolution; //miejsce na zapisanie najlepszej dotychczasowej sekwencji
    int savedCost = INT_MAX; //najmniejszy dotychczasowy koszt
    int cost;
    double time;
    startCounter();
    int i = 0;
    bool check;
    do{
        cost = 0;
        cost += edges[0][nodes.front()];  //dodanie wagi krawędzi 0 - pierwszy wierzchołek z permutacji
        for (int i = 1; i < nodeCount-1; i++){
            cost+= edges[nodes[i-1]][nodes[i]]; //dodawanie kolejnych krawędzi
        }
        cost += edges[nodes.back()][0]; //dodanie wagi krawędzi z ostatniego wierzchołka permutacji do 0
        if (cost < savedCost) {
            savedCost = cost;
            savedSolution = nodes;
        }
        check = next_permutation(nodes.begin(), nodes.end());
    } while (check); //jeżeli istnieje kolejna permutacja - kolejna pętla, jeżeli nie - kończy się
    time = getCounter();
    if (a == 0) {
        //cout << savedCost << " [0";
        for (int i = 0; i < nodeCount - 1; i++) {
            //cout << " " << savedSolution[i];
        }
        //cout << " 0]; ";
        saveFile << savedCost << " [0";
        for (int i = 0; i < nodeCount-1; i++){
            saveFile << " " << savedSolution[i];
        }
        saveFile << " 0]" << endl;
    }
    //cout << time << endl;
    saveFile << time << endl;
    nodes.clear();
}

int main() {
    string fileName;
    int n;
    ofstream saveFile("results.csv");
    fileName = "tsp_6_1.txt";
    n = 1;
    readFile(fileName);
    //cout << fileName << endl;
    saveFile << fileName << " " << n << " ";
    for (int i = 0; i < n; i++){
        bruteforce(saveFile, i);
    }
    fileName = "tsp_6_2.txt";
    readFile(fileName);
    //cout << fileName << endl;
    saveFile << fileName << " " << n << " ";
    for (int i = 0; i < n; i++){
        bruteforce(saveFile, i);
    }
    fileName = "tsp_10.txt";
    readFile(fileName);
    //cout << fileName << endl;
    saveFile << fileName << " " << n << " ";
    for (int i = 0; i < n; i++){
        bruteforce(saveFile, i);
    }
    fileName = "tsp_12.txt";
    readFile(fileName);
    //cout << fileName << endl;
    saveFile << fileName << " " << n << " ";
    for (int i = 0; i < n; i++){
        bruteforce(saveFile, i);
    }
    fileName = "tsp_13.txt";
    readFile(fileName);
    //cout << fileName << endl;
    saveFile << fileName << " " << n << " ";
    for (int i = 0; i < n; i++){
        bruteforce(saveFile, i);
    }
    return 0;
}
