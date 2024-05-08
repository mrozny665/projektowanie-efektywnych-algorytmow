#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>
#include <windows.h>

using namespace std;

int N;
vector<vector<int>> adjacencyMatrix;
vector<string> fileNames;
vector<int> repeatCounts;
vector<int> solutions;
vector<string> paths;
string outputFileName;

//blok mierzenia czasu

double PCFreq = 0.0;
__int64 CounterStart = 0;

void startCounter() {
    LARGE_INTEGER li;
    if( !QueryPerformanceFrequency( & li ) )
        printf("QueryPerformanceFrequency failed!\n");

    PCFreq = double( li.QuadPart ) / 1000.0;     // sekundy podzielone przez tysiac - milisekundy

    QueryPerformanceCounter( & li );
    CounterStart = li.QuadPart;
}
double getCounter() {
    LARGE_INTEGER li;
    QueryPerformanceCounter( & li );
    return double( li.QuadPart - CounterStart ) / PCFreq;
}

//koniec bloku mierzenia czasu

vector<int> simulatedAnnealing(bool geometric, bool swap);

void readIniFile(const string& fileName){
    ifstream file(fileName);
    string line;

    string newFileName;
    int numberOfTests;
    int solution;
    string trace;

    if (file.good())
    {
        while (getline(file, line))
        {
            stringstream sstream(line);
            sstream >> newFileName;

            if (newFileName.find("PEA") != string::npos)
            {
                outputFileName = newFileName;
                break;
            }
            fileNames.push_back(newFileName);
            sstream >> numberOfTests;
            repeatCounts.push_back(numberOfTests);
            sstream >> solution;
            solutions.push_back(solution);
            getline(sstream, trace);
            paths.push_back(trace);
        }
    }
    else  printf("Wystapil problem podczas otwierania pliku inicjujacego");
}

void readFile(const string& fileName){
    ifstream file("./files2/" + fileName);
    string line;
    int value;
    vector <int> row;

    int rowCounter = 0;

    if (file.good())
    {
        while (getline(file, line))
        {
            stringstream str(line);
            if (rowCounter == 0)	// pierwszy rzad zawiera liczbe wierzcholkow
            {
                str >> value;
                N = value;
                rowCounter++;
                continue;
            }

            row.resize(0);	// czysczenie wiersza
            adjacencyMatrix.push_back(row);

            for (int j = 0; j < N; j++)	// j - column
            {
                str >> value;
                adjacencyMatrix[rowCounter - 1].push_back(value);
            }
            rowCounter++;
        }
    }
    else printf("Wystapil problem podczas otwierania pliku testu: %s", fileName.c_str());
}

int calculateTotalDistance(const vector<int>& tour) {
    int totalDistance = 0;
    for (int i = 0; i < N - 1; ++i) {
        totalDistance += adjacencyMatrix[tour[i]][tour[i + 1]];
    }
    totalDistance += adjacencyMatrix[tour[N - 1]][tour[0]];
    return totalDistance;
}

vector<int> generateRandomTour() { //pozostałość po początkowych etapach
    vector<int> tour(N);
    for (int i = 0; i < N; ++i) {
        tour[i] = i;
    }
    shuffle(tour.begin() + 1, tour.end(), std::mt19937(std::random_device()())); // Shuffle excluding the starting city
    return tour;
}

vector<int> generateGreedyTour(){ //tworzenie rozwiązania początkowego metodą zachłanną
    vector<int> tour;
    bool visited[N];
    for (int i = 0; i < N; i++){
        visited[i] = false;
    }
    int nextCity;
    int nextDistance;
    tour.push_back(0); //dodanie miasta początkowego
    visited[0] = true;
    for (int i = 0; i < N - 1; i++){
        nextDistance = INT_MAX;
        for (int j = 0; j < N; j++){ //porównywanie kolejnych wierzchołków
            if (tour.back() == j) continue;
            if (visited[j]) continue;
            if (adjacencyMatrix[tour.back()][j] < nextDistance){ //jeżeli badana krawędź jest lepsza od najlepszej znalezionej jest ona zachowywana
                nextDistance = adjacencyMatrix[tour.back()][j];
                nextCity = j;
            }
        }
        tour.push_back(nextCity); //do rozwiązania dodawany jest wierzchołek, który miał najmniejszą wartość drogi
        visited[nextCity] = true;
    } //powtarza się to aż do wyczerpania listy wierzchołków
    return tour;
}

vector<int> generateRandomNeighbor(vector<int> tour, bool swap){ //generowanie śąsiadów
    int a = (rand() % (N-1)) + 1, b;
    do b = (rand() % (N-1)) + 1; while (b == a); //losowane są dwa różne wierzchołki
    if (swap) iter_swap(tour.begin() + a, tour.begin() + b);
    else reverse(tour.begin() + a, tour.begin() + b); //w zależności od warunków są zamieniane lub odwracane
    return tour;
}

void runTests(bool geometric, bool swap){
    string newFileName = outputFileName;
    if (geometric) newFileName += "-geometric";
    else newFileName += "-logarithmic";
    if (swap) newFileName += "-swap";
    else newFileName += "-reverse";
    newFileName += ".csv";
    ofstream outputFile(newFileName);
    int k = 0;
    for (const auto & fileName : fileNames){
        vector <int> bestTour;
        int bestDistance = INFINITY;
        readFile(fileName);
        printf("%s\n", fileName.c_str());
        outputFile << fileName << endl;
        for (int i = 0; i < repeatCounts[k]; i++){
            startCounter();
            vector<int> currentTour = simulatedAnnealing(geometric, swap);
            double time = getCounter();
            int currentDistance = calculateTotalDistance(currentTour);
            if (currentDistance < bestDistance) {
                bestTour = currentTour;
                bestDistance = currentDistance;
            }
            float error = (float) currentDistance / (float) solutions[k];
            printf("%f; %i; %f; [", time, currentDistance, error);
            outputFile << time << "; " << currentDistance << "; " << error << "; [";
            for (int j = 0; j < N; j++){
                printf("%i ", bestTour[j]);
                outputFile << bestTour[j] << " ";
            }
            printf("%i]\n", bestTour[0]);
            outputFile << bestTour[0] << "]" << endl;
        }
        k++;
        adjacencyMatrix.resize(0);
    }
}

float cooling = 0.9; //stała chłodzenia musi być bliska, ale mniejsza od 1, tu ustalona na 0.9

double geometricCooling(double temp){
    return temp * cooling; //chłodzenie geometryczne - mnożenie temperatury przez stałą
}

double logarithmicCooling(double temp, int k){
    return temp / (1 + log (1+k)); //chłodzenie logarytmiczne - dzielenie temperatury przez mianownik,
                                        //w jego składzie logarytm
}

vector<int> simulatedAnnealing(bool geometric, bool swap) {
    vector <int> currentTour;
    vector <int> neighbor;
    vector <int> bestTour;
    int currentLength;
    int neighborLength;
    int bestLength;
    int epochLength;
    double temperature;

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    currentTour = generateGreedyTour(); //generowanie rozwiązania początkowego
    currentLength = calculateTotalDistance(currentTour);
    temperature = (double) currentLength * N; //ustalenie temperatury początkowej - wartość rozwiązania początkowego
                                              //podzielona przez liczbę wierzchołków
    bestTour = currentTour;
    bestLength = currentLength;
    epochLength = N * 1000; //ustalenie długości epoki - liczba wierzhcołków razy 1000
    for (int i = 0; ; i++){ //pętla zewnętrzna - zależna od temperatury
        if (temperature < 0.1) { //warunek końcowy - jeżeli temperatura spadnie poniżej 0.1
            break;
        }
        for (int j = 0; j < epochLength; j++) { // pętla wewnętrzna - zależna od długości epoki
            neighbor = generateRandomNeighbor(currentTour, swap); //tworzenie nowego rozw. z sąsiedztwa
            neighborLength = calculateTotalDistance(neighbor);
            if (neighborLength < currentLength) { //jeżeli nowe rozw. jest lepsze od starego - jest zapisywane
                currentTour = neighbor;
                currentLength = neighborLength;
            } else { //jeżeli nowe rozw. jest gorsze następuje wyliczenie prawdopodobieństwa
                float probability = exp(((float) neighborLength - (float) currentLength) / temperature);
                if (dist(mt) < probability) {
                    currentTour = neighbor;
                    currentLength = neighborLength;
                }
            }
            if (currentLength < bestLength) { //porównanie obecnego rozw. z najlepszym dotychczas znalezionym
                bestTour = currentTour;
                bestLength = currentLength;
            }
        } //poniżej - chłodzenie
        geometric ? temperature = geometricCooling(temperature) : temperature = logarithmicCooling(temperature, i);
    }
    return bestTour;
}


int main() {
    readIniFile("PEA_SA.ini");
    readFile(fileNames[0]);
    runTests(true, true);
    runTests(true, false);
    runTests(false, true);
    runTests(false, false);
    return 0;
}

