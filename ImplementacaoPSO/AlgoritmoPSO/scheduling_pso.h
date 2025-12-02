#ifndef SCHEDULING_PSO_H
#define SCHEDULING_PSO_H

#include "ModeloProblema.h"
#include <random>
#include <cmath>
#include <algorithm>
#include <chrono>

using namespace std;

// ===== ESTRUTURAS PARA PSO =====

// Estrutura para representar uma partícula
struct Particle {
    vector<int> position; // Posição atual (permutação)
    vector<int> bestPosition; // Melhor posição encontrada
    vector<int> velocity; // Velocidade (sequência de movimentos)
    double fitness; // Fitness atual
    double bestFitness; // Melhor fitness encontrado

    Particle() : fitness(0.0), bestFitness(numeric_limits<double>::max()) {
    }
};

// Estrutura para estatísticas de cada geração
struct GenerationStats {
    int generation;
    double bestFitness;
    double avgFitness;
    double worstFitness;
    double elapsedTime;
};

// ===== CLASSE PSO =====
class PSO {
private:
    // Parâmetros do PSO
    int populationSize; // P_s (tamanho da população)
    int numGenerations; // Número máximo de gerações
    double c1; // Coeficiente de aprendizado (local best)
    double c2; // Coeficiente de aprendizado (global best)
    double inertiaWeight; // Peso de inércia
    double mutationProb; // Probabilidade de mutação
    int crossoverType; // Tipo de crossover (1=OP, 2=TP, 3=PMX, 4=PTL)
    int mutationOperator; // Tipo de mutação (1=Swap, 2=Insert, 3=MultiSwap, 4=MultiInsert)

    // Dados
    ProblemData problemData;
    vector<Particle> swarm;
    Particle globalBest;
    vector<GenerationStats> generationHistory;

    // RNG
    mt19937 rng;
    uniform_real_distribution<double> uniformDist;

    // Métodos auxiliares
    void initializeSwarm();

    double evaluateParticle(vector<int> &position);

    void resetProblemData();

    void copyProblemData(ProblemData &source, ProblemData &dest);

    // Operadores de crossover
    vector<int> orderCrossover(const vector<int> &parent1, const vector<int> &parent2);

    vector<int> twoPointCrossover(const vector<int> &parent1, const vector<int> &parent2);

    vector<int> pmxCrossover(const vector<int> &parent1, const vector<int> &parent2);

    vector<int> ptlCrossover(const vector<int> &parent1, const vector<int> &parent2);

    // Operadores de mutação
    void swapMutation(vector<int> &solution);

    void insertMutation(vector<int> &solution);

    void multiSwapMutation(vector<int> &solution);

    void multiInsertMutation(vector<int> &solution);

    // ILS - Iterated Local Search
    void ilsLocalSearch(vector<int> &solution);

    // Operadores de aprendizado
    void learnFromHistoryMutation(vector<int> &position);

    void learnFromLocalBestCrossover(const vector<int> &position, const vector<int> &localBest,
                                     vector<int> &newPosition);

    void learnFromGlobalBestCrossover(const vector<int> &position, const vector<int> &globalBest,
                                      vector<int> &newPosition);

public:
    PSO(int popSize, int numGen, double c1_val, double c2_val, double inertia,
        double mutProb, int crossType, int mutType);

    ~PSO();

    // Executar o algoritmo
    void run(const string &instanceFile, const string &outputFile);

    // Setters
    void setPopulationSize(int size) { populationSize = size; }
    void setNumGenerations(int gen) { numGenerations = gen; }
    void setC1(double val) { c1 = val; }
    void setC2(double val) { c2 = val; }

    // Getters
    const Particle &getGlobalBest() const { return globalBest; }
    const vector<GenerationStats> &getHistory() const { return generationHistory; }
    // Método para obter o vetor bestPosition do global best

    // Método conveniente para retornar como string
    string getGlobalBestPositionString() const {
        ostringstream oss;
        const auto &pos = globalBest.bestPosition;
        for (size_t i = 0; i < pos.size(); i++) {
            if (i > 0) oss << "-";
            oss << pos[i];
        }
        return oss.str();
    }
};


#endif // SCHEDULING_PSO_H
