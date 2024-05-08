#include "algorithm"
#include <cmath>
#include <random>
#include <fstream>
#include <chrono>
#include <sstream>

using namespace std;
//TODO zapisywać ścieżkę do pliku
//TODO spróbować na cztery
vector<vector<int>> adjacencyMatrix;
string outputFileName;
vector<string> fileNames;
vector<int> repeatCounts;
vector<int> solutions;
vector<string> paths;
vector<int> pherUpdates;
int N;

vector<vector<double>> CAS(double pher, int l, vector<vector<double>> pher_to_add, vector<int> path);
vector<vector<double>> QAS(double pher, vector<vector<double>> pher_to_add, vector<int> path);
vector<vector<double>> DAS(double pher, vector<vector<double>> pher_to_add, vector<int> path);

int calc_cost(std::vector<int> &path, vector<vector<int>> &dist){
    int cost = 0;
    for(int i =0; i < path.size()-1; i++)
        cost += dist[path[i]][path[i+1]];
    return cost;
}

int calc_init_Cnn(int vert, vector<vector<int>> &dist){

    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<int> d(1, vert-1);

    vector<int> path;
    for( int k=0; k < vert; k++)
        path.push_back(k);
    path.push_back(0);

    int sum = 0;
    int N = 10000;
    for(int i = 0; i < N; i++){
        std::shuffle(path.begin()+1 ,path.end()-1, g);
        sum += calc_cost(path, dist);
    }
    sum = (int)((double) sum / (double) N);
    return sum;
}

vector<int> calculate(int vert, vector<vector<int>> &dist, int p, chrono::time_point<chrono::high_resolution_clock> start, int pherUpdt) {
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_real_distribution<double> num(0.0, 1.0);
    std::uniform_int_distribution<int> node(0, vert-1);
    vector<int> best_path;
    int best_cost = INT_MAX;
    int currBestCost = INT_MAX;

    double alpha = 1.0;
    double beta = 2.5;
    double evaporation = 0.5; // ro

    int Cnn = calc_init_Cnn(vert,dist);
    double init_pher = (double) vert / (double) Cnn; // poczatkowa ilosc feromonow
    double pher = 100.0; // ilosc feromonu zostawianego przez mrowke
    // macierz feromonow
    vector<vector<double>> pheromones;
    for(int i = 0; i < vert; i++){
        vector<double> ph;
        for(int k=0; k < vert; k++)
            ph.push_back(init_pher);
        pheromones.push_back(ph);
    }

    vector<vector<double>> pheromones_to_add;
    for(int i = 0; i < vert; i++){
        vector<double> ph;
        for(int k=0; k < vert; k++)
            ph.push_back(0.0);
        pheromones_to_add.push_back(ph);
    }
    int i = 0;
    double condition;
    if (N < 25) condition = solutions[p];
    else if (N < 74) condition = solutions[p] * 1.5;
    else if (N < 449) condition = solutions[p] * 2;
    else condition = solutions[p] * 2.5;
    int stayCount = 0;
    while (condition < best_cost){
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::minutes>(stop - start);
        if (duration >= std::chrono::minutes(10)) break;
        if (stayCount >= 1000) break;
        printf("ITERACJA: %d\n", ++i);
        // znalezienie m cyklów przez m mrówek
        for(int m = 0; m < vert; m++){
            // dla mrowki losowany jest wierzchołek startowy
            int start_node = m;
            vector<int> path;
            path.push_back(start_node);   // sciezka danej mrowki
            vector<int> left_vertices;	// lista wierzchołków mozliwych do wybrania

            for (int y = 0; y < vert; y++){ // uzupelnienie listy wszystkimi mozliwymi
                if (y != start_node)
                    left_vertices.push_back(y);
            }
            // szukanie sciezki
            for (int x = 0; x < vert-1 ; x++){
                int cn = path.back();
                //obliczenie prawopodobienst wybrania pozostalych wierzcholkow
                double sum_factor = 0.0; // dolne wyrazenie w obliczaniu prawdo
                for(int & leftVertex : left_vertices) {
                    double a = dist[cn][leftVertex];
                    if (a == 0) a = 0.1;
                    sum_factor += (pow(pheromones[cn][leftVertex], alpha)) *
                                  (pow(1.0/(double) a, beta));
                }
                vector<double> probabil;

                for(int & leftVertex : left_vertices) {
                    double a = dist[cn][leftVertex];
                    if (a == 0) a = 0.1;
                    double p = ((pow(pheromones[cn][leftVertex], alpha))
                            * (pow(1.0/(double) a, beta))) / sum_factor;
                    probabil.push_back(p);
                }
                // Roulette-Wheel-Selection
                partial_sum(probabil.begin(),probabil.end(),probabil.begin());
                double p = num(g); // wylosowanie liczby z przedzialu [0,1]

                int k = 0;

                for(double & ix : probabil){

                    if(p < ix){
                        // wybrano nastepny wierzcholek
                        int wybrany = left_vertices[k];
                        path.push_back(wybrany);
                        left_vertices.erase(left_vertices.begin()+k);
                        // usuniecie z dostepnych wierzchołków
                        break;
                    }
                    k++;
                }
                if (pherUpdt == 0) pheromones_to_add = DAS(pher, pheromones_to_add, path);
                if (pherUpdt == 1) pheromones_to_add = QAS(pher, pheromones_to_add, path);

            }
            path.push_back(start_node);
            // Obliczenie kosztu znalezionego cyklu
            int L = calc_cost(path,dist);
            if( L < best_cost){
                best_cost = L;
                best_path = path;
            }
            if (pherUpdt == 2) pheromones_to_add = CAS(pher, L, pheromones_to_add, path);
        }
        // Uaktualnienie ilosc feromonow na kazdej krawedzi
        for(int j = 0; j < vert; j++){
            for(int k=0; k < vert; k++){
                pheromones[j][k] = (pheromones[j][k] * evaporation) + pheromones_to_add[j][k];
                pheromones_to_add[j][k] = 0.0;
            }
        }
        if (currBestCost > best_cost) currBestCost = best_cost;
        else stayCount++;

    }
    best_path.pop_back();
    auto itt = find(best_path.begin(), best_path.end(), 0);
    std::rotate(best_path.begin(),itt,best_path.end());
    best_path.push_back(0);
    vector<int> solution_and_cost(best_path);
    solution_and_cost.push_back(best_cost);
    return solution_and_cost;
}

vector<vector<double>> CAS(double pher, int l, vector<vector<double>> pher_to_add, vector<int> path) {
    // obliczenie ile feromonow dodac
    double ph = pher/(double) l;
    // dodanie pheromonow tylko na sciezka przez ktore mrowka szla
    for(int r =0; r < path.size()-1; r++)
        pher_to_add[path[r]][path[r+1]] += ph;
    return pher_to_add;
}

vector<vector<double>> QAS(double pher, vector<vector<double>> pher_to_add, vector<int> path) {
    int city1 = path[path.size()-2];
    int city2 = path[path.size()-1];
    double ph = pher/adjacencyMatrix[city1][city2];
    pher_to_add[city1][city2] = ph;
    return pher_to_add;
}

vector<vector<double>> DAS(double pher, vector<vector<double>> pher_to_add, vector<int> path) {
    int city1 = path[path.size()-2];
    int city2 = path[path.size()-1];
    pher_to_add[city1][city2] = pher;
    return pher_to_add;
}

#define FILE_IN "PEA_ACO.ini"

void readIniFile(const string& fileName){
    ifstream file(fileName);
    string line;

    string newFileName;
    int numberOfTests;
    int solution;
    string trace;
    int pher;

    if (file.good())
    {
        while (getline(file, line))
        {
            stringstream sstream(line);
            sstream >> newFileName;

            if (newFileName.find(".csv") != string::npos)
            {
                outputFileName = newFileName;
                break;
            }
            fileNames.push_back(newFileName);
            sstream >> pher;
            pherUpdates.push_back(pher);
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
    ifstream file("./files/" + fileName);
    string line;
    int value;
    vector <int> row;

    int rowCounter = 0;
    adjacencyMatrix.resize(0);

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
    file.close();
}

int main(){
    readIniFile(FILE_IN);
    std::ofstream file_out(outputFileName);
    vector <double> bledyy;
    for (int i = 0; i < fileNames.size(); i++) {
        vector<int> bestSolutionAndCost(N + 2);
        bestSolutionAndCost[bestSolutionAndCost.size() - 1] = INT_MAX;
        readFile(fileNames[i]);
        // Wykonanie n powtórzeń algorytmu szukania optymalnego cyklu
        int rep = 1;
        printf("%s                     nodes: %d\n", fileNames[i].c_str(), N);
        int sr = 0;
        double bledy = 0.0;
        vector<int> results;
        while (rep++ <= repeatCounts[i]) {
            std::vector<int> solution_cost;

            auto start = std::chrono::high_resolution_clock::now();

            solution_cost = calculate(N, adjacencyMatrix, i, start, pherUpdates[i]);

            auto stop = std::chrono::high_resolution_clock::now();

            if (solution_cost[solution_cost.size()-1] < bestSolutionAndCost[bestSolutionAndCost.size()-1])
                bestSolutionAndCost = solution_cost;


            // wpisanie do pliku csv danych instancji w celu określenia dla jakiej instancji będą podane czasy
            // podanie obliczonego kosztu oraz sciezki optymalnej dla 1 powtorzenia
            if (rep == 2) {
                file_out << fileNames[i] + "; "  << N << "; ";
            }

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
            file_out << duration.count() << "; ";              // wpisanie czasu do pliku csv
            results.push_back(solution_cost[N + 1]);
        }
        file_out << endl << fileNames[i] + "; " << N << "; ";
        for (int result : results) {
            int best_cost = solutions[i];
            if (result != best_cost) {             // jeśli algorytm wyznaczył inny koszt podajemy go
                double temp = ((double) (result - best_cost) / (double) best_cost) * 100.0;
                file_out << temp << " %; ";
                printf("blad: %f %%; ", temp);
            } else {
                file_out << "opt; ";
                printf("koszt optymalny; ");
            }
            sr++;
            bledy += ((double) (result - best_cost) / (double) best_cost) * 100.0;
        }
        file_out << endl;
        std::string ret_path;
        for (int j = 0; j < N + 1; j++) {
            ret_path += (std::to_string(bestSolutionAndCost[j]) + " ");
        }
        file_out << ret_path << endl;
        double temp = bledy/sr;
        printf("sredni blad (w %%) dla aktualnej instancji ponizej\n%f\n", temp);
        bledyy.push_back(temp);

        printf("__________________________________________________\n");
    }
    printf("SREDNIE BLEDY w %% DLA KOLEJNO WSZYSTKICH INSTANCJI\n");
    for (double z : bledyy){
        printf("%f\n", z);
    }
    file_out.close();
}