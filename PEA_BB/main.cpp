#include<iostream>			// wejscie, wyjscie
#include<vector>            // wektory
#include<queue>			    // kolejka priorytetowa
#include<fstream>			// operacje na plikach
#include<sstream>
#include<algorithm>			// std::sort, std::find
#include<windows.h>         // potrzebne do pomiaru czasu
#include<cmath>

using namespace std;

#define INIT_FILE "PEA_BB.ini"

// zmienne/struktury do BB
vector< vector<vector<int>> > reducedMatrixes;

struct Node
{
    int numberOfNode{};
    int numberOfCity{};
    int level{};
    int cost{};
    vector<int> listOfVisitedCities;

};

// podstawowe struktury
int N;										// liczba wierzcholkow
vector < vector<int> > adjacencyMatrix;		// macierz sasiedztwa
vector <int> outcome;						// trasa znalezionego rozwiazania
int minSum = INFINITY;						// minimum globalne (sumy wag krawedzi)

// struktury do testow
vector <string> fileNameVector;
vector <int> numberOfTestsVector;
vector <int> solutionVector;
vector <string> traceVector;
string outputFileName;

//blok mierzacy czas - wg zalecen dra Mierzwy ze SDiZO

double PCFreq = 0.0;
__int64 CounterStart = 0;

void startCounter() {
    LARGE_INTEGER li;
    if( !QueryPerformanceFrequency( & li ) )
        cout << "QueryPerformanceFrequency failed!\n";

    PCFreq = double( li.QuadPart ) / 1000.0;     // sekundy podzielone przez tysiac - milisekundy

    QueryPerformanceCounter( & li );
    CounterStart = li.QuadPart;
}
double getCounter() {
    LARGE_INTEGER li;
    QueryPerformanceCounter( & li );
    return double( li.QuadPart - CounterStart ) / PCFreq;
}
//koniec bloku do mierzenia czasu

// funkcja odczytujaca pliki *.ini
void readIniFile(const string& FileName)
{
    fstream file;

    file.open(FileName.c_str(), ios::in);
    string line;

    string fileName;
    int numberOfTests;
    int solution;
    string trace;
    string outputFileExtension = ".csv";

    if (file.good())
    {
        while (getline(file, line))
        {
            stringstream sstream(line);
            sstream >> fileName;

            if (fileName.find(outputFileExtension) != string::npos)
            {
                outputFileName = fileName;
                break;
            }
            fileNameVector.push_back(fileName);
            sstream >> numberOfTests;
            numberOfTestsVector.push_back(numberOfTests);
            sstream >> solution;
            solutionVector.push_back(solution);
            getline(sstream, trace);
            traceVector.push_back(trace);
        }
    }
    else  cout << "Wystapil problem podczas otwierania pliku inicjujacego";
}

void readTestFile(const string& fileName)
{
    fstream file;

    file.open(fileName.c_str(), ios::in);
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
    else  cout << "Wystapil problem podczas otwierania pliku testu:" << fileName;
}

string vectorOfVisitedVertexesToString(vector <int> vectorOfVisitedVertexes)
{
    string visitedString;

    for (int i = (int) vectorOfVisitedVertexes.size() - 1; i >= 0; i--)		// odczytujemy od tylu bo wierzcholki byly pushowane od tylu
    {
        if (i != vectorOfVisitedVertexes.size() - 1 && i != 0) visitedString += to_string(vectorOfVisitedVertexes[i]) + " ";
        else if (i == vectorOfVisitedVertexes.size() - 1) visitedString += "[" + to_string(vectorOfVisitedVertexes[i]) + " ";
        else visitedString += to_string(vectorOfVisitedVertexes[i]) + "]";
    }

    return visitedString;
}

Node* createReducedMatrix(Node* parentNode, int numberOfCurrentCity)
{
    int reducedValue;

    Node* newNode = new Node;

    newNode->numberOfCity = numberOfCurrentCity;

    if (numberOfCurrentCity == 0)
    {
        vector<int> visitedCities;
        newNode->listOfVisitedCities = visitedCities;
        newNode->level = 1;
    }
    else
    {
        newNode->listOfVisitedCities = parentNode->listOfVisitedCities;
        newNode->listOfVisitedCities.push_back(numberOfCurrentCity);
    }

    newNode->numberOfNode = (int) reducedMatrixes.size();


    // tworzenie macierzy do pozniejszego redukowania
    vector<vector<int>> tempMatrix;
    for (int i = 0; i < N; i++)
    {
        vector<int> tempVector;
        for (int j = 0; j < N; j++)
        {
            if(numberOfCurrentCity == 0 ) tempVector.push_back(adjacencyMatrix[i][j]);
            else tempVector.push_back(reducedMatrixes[parentNode->numberOfNode][i][j]);
        }
        tempMatrix.push_back(tempVector);
        tempVector.resize(0);
    }

    reducedMatrixes.push_back(tempMatrix);
    tempMatrix.resize(0);

    if (numberOfCurrentCity != 0)
    {
        for (int i = 0; i < N; i++) reducedMatrixes[newNode->numberOfNode][i][numberOfCurrentCity] = INFINITY;
        for (int j = 0; j < N; j++) reducedMatrixes[newNode->numberOfNode][parentNode->numberOfCity][j] = INFINITY;
        reducedMatrixes[newNode->numberOfNode][numberOfCurrentCity][parentNode->numberOfCity] = INFINITY;
    }

    // redukowanie wierszy
    int reducedValueInRows = 0;
    for (int i = 0; i < N; i++)	// dla kazdego wiersza
    {
        int minInRow = INFINITY;
        // znajdowanie minimum w wierszu
        for (int j = 0; j < N; j++) // dla kazdej wartosci w wierszu
        {
            if (i == j or reducedMatrixes[newNode->numberOfNode][i][j] == (int) INFINITY) continue;	// pomijanie nieskonczonosci i zer
            if (reducedMatrixes[newNode->numberOfNode][i][j] < minInRow) minInRow = reducedMatrixes[newNode->numberOfNode][i][j];
        }

        if (minInRow == 0 or minInRow == (int) INFINITY) continue;

        reducedValueInRows += minInRow;

        // zmiana wartosci w rzedach
        for (int j = 0; j < N; j++) // dla kazdej wartosci w wierszu
        {
            if (i == j or reducedMatrixes[newNode->numberOfNode][i][j] == (int) INFINITY) continue;	// pomijanie nieskonczonosci i zer
            reducedMatrixes[newNode->numberOfNode][i][j] -= minInRow;			// tworzenie czesciowo zredukowanej macierzy
        }
    }

    // redukowanie kolumn - analogicznie do rzedow
    int reducedValueInColumns = 0;
    for (int j = 0; j < N; j++)
    {
        int minInColumn = INFINITY;

        for (int i = 0; i < N; i++)
        {
            if (i == j or reducedMatrixes[newNode->numberOfNode][i][j] == (int) INFINITY) continue;
            if (reducedMatrixes[newNode->numberOfNode][i][j] < minInColumn) minInColumn = reducedMatrixes[newNode->numberOfNode][i][j];
        }

        if (minInColumn == 0 or minInColumn == (int) INFINITY) continue;

        reducedValueInColumns += minInColumn;


        for (int i = 0; i < N; i++)
        {
            if (i == j or reducedMatrixes[newNode->numberOfNode][i][j] == (int) INFINITY) continue;
            reducedMatrixes[newNode->numberOfNode][i][j] -= minInColumn; //tworzenie wlasciwej zredukowanej macierzy
        }
    }

    //obliczanie kosztu redukcji
    reducedValue = reducedValueInRows + reducedValueInColumns;
    int cost;
    if (numberOfCurrentCity == 0)  cost = reducedValue;
    else  cost = reducedMatrixes[parentNode->numberOfNode][parentNode->numberOfCity][numberOfCurrentCity] + reducedValue + parentNode->cost;	// C(1,2) reducedValue parentReducedValue
    newNode->cost = cost;

    return newNode;
}
// porownanie dla kolejki priorytetowej
struct Comp {
    bool operator()(const Node* a, const Node* b) {
        return a->cost > b->cost;
    }
};


void BranchBound()
{
    priority_queue< Node*, vector<Node*>, Comp> pq;

    vector<int> tempTrace;

    pq.push(createReducedMatrix(nullptr, 0));

    while (!pq.empty())
    {
        Node* nodeFromQ = pq.top();
        pq.pop();

        if (nodeFromQ -> level == N)
        {
            minSum = nodeFromQ -> cost;
            tempTrace = nodeFromQ -> listOfVisitedCities;
            break;
        }

        for (int i = 1; i < N; i++)
        {
            if (find(nodeFromQ -> listOfVisitedCities.begin(), nodeFromQ -> listOfVisitedCities.end(), i) != nodeFromQ -> listOfVisitedCities.end()) continue; // If city is visited then continue

            Node* node = createReducedMatrix(nodeFromQ, i);
            node -> level = nodeFromQ -> level+1;
            pq.push(node);
        }

        delete nodeFromQ;
    }

    outcome.push_back(0);
    for (int i = 0; i < tempTrace.size(); i++) outcome.push_back(tempTrace[tempTrace.size()-1-i]);
    outcome.push_back(0);
    tempTrace.resize(0);
}

void hardClear()
{
    minSum = INFINITY;
    adjacencyMatrix.resize(0);									// clear adjacency matrix
    outcome.resize(0);
    reducedMatrixes.resize(0);
}

void softClear()												// without clearing adjacencyMatrix
{
    minSum = INFINITY;
    outcome.resize(0);
    reducedMatrixes.resize(0);
}


void runTests()
{
    ofstream myfile;
    myfile.open(outputFileName);		// open csv file
    double time;
    string trace;

    hardClear();

    for (int i = 0; i < fileNameVector.size(); i++)
    {
        myfile << fileNameVector[i] << ";" << numberOfTestsVector[i] << ";" << solutionVector[i] << ";" << traceVector[i];
        printf("%s;%d;%d;%s", fileNameVector[i].c_str(), numberOfTestsVector[i], solutionVector[i], traceVector[i].c_str());

        readTestFile(fileNameVector[i]);	// read test data

        myfile << "\n";
        printf("\n");

        for (int j = 0; j < numberOfTestsVector[i]; j++)
        {
            startCounter();
            BranchBound();
            time = getCounter();

            trace = vectorOfVisitedVertexesToString(outcome);

            myfile << time << ";" << minSum << ";" << trace << "\n";
            printf("%f;%d;%s\n", time, minSum, trace.c_str());
            softClear();
        }

        hardClear();  // reset everything after reading one test file

        myfile << "\n";
        printf("\n");
    }

    myfile.close();
}

int main()
{
    readIniFile(INIT_FILE);
    runTests();
    return 0;
}