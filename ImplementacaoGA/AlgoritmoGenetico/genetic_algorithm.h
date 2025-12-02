#ifndef GENETIC_ALGORITHM_H
#define GENETIC_ALGORITHM_H

#include "../scheduling_ga.h"
#include <random>
#include <chrono>
#include <set>

using namespace std;

enum class SelectionType
{
    TOURNAMENT,
    ROULETTE_WHEEL
};

enum class CrossoverType
{
    OBX,
    PMX,
    SB2OX,
    OPX,
    TPX
};

enum class MutationType
{
    INSERT,
    INTERCHANGE,
    SWAP
};

struct GAParameters
{
    SelectionType selectionType;
    CrossoverType crossoverType;
    MutationType mutationType;

    int populationSize;
    double crossoverProb;
    double mutationProb;
    int restartGenerations;
    int localSearchFreq;
    int localSearchIntensity;
    double maxCPUTimeSeconds;

    GAParameters()
        : selectionType(SelectionType::TOURNAMENT),
          crossoverType(CrossoverType::OBX),
          mutationType(MutationType::INSERT),
          populationSize(70),
          crossoverProb(0.95),
          mutationProb(0.03),
          restartGenerations(50),
          localSearchFreq(10),
          localSearchIntensity(1),
          maxCPUTimeSeconds(60.0) {}
};

struct Individual
{
    vector<int> chromosome;
    double fitness;

    Individual() : fitness(numeric_limits<double>::max()) {}
    Individual(const vector<int> &chr) : chromosome(chr), fitness(numeric_limits<double>::max()) {}

    bool operator<(const Individual &other) const
    {
        return fitness < other.fitness;
    }
};

// Estrutura para armazenar histórico de uma geração
struct GenerationStats
{
    int generation;
    double bestFitness;
    double avgFitness;
    double worstFitness;
    double elapsedTime;
};

class GeneticAlgorithm
{
private:
    GAParameters params;
    ProblemData problemData;
    vector<Individual> population;
    Individual bestSolution;
    int currentGeneration;
    int generationsWithoutImprovement;
    mt19937 rng;

    // Histórico de gerações
    vector<GenerationStats> history;

    void initializePopulation();
    void initializePopulationWithSeed(const vector<int> &seedChromosome);
    void evaluateIndividual(Individual &ind);
    void evaluatePopulation();
    void recordGenerationStats(double elapsedTime);

    vector<Individual> tournamentSelection();
    vector<Individual> rouletteWheelSelection();
    vector<Individual> performSelection();

    pair<Individual, Individual> orderBasedCrossover(const Individual &p1, const Individual &p2);
    pair<Individual, Individual> partialMappedCrossover(const Individual &p1, const Individual &p2);
    pair<Individual, Individual> similarBlock2PointCrossover(const Individual &p1, const Individual &p2);
    pair<Individual, Individual> onePointOrderCrossover(const Individual &p1, const Individual &p2);
    pair<Individual, Individual> twoPointOrderCrossover(const Individual &p1, const Individual &p2);
    pair<Individual, Individual> performCrossover(const Individual &p1, const Individual &p2);

    void insertMutation(Individual &ind);
    void interchangeMutation(Individual &ind);
    void swapMutation(Individual &ind);
    void performMutation(Individual &ind);

    void localSearch(Individual &ind);
    void restartProcedure();
    void halfGenesMutation(Individual &ind);

    bool isDuplicate(const Individual &ind);
    int getWorstIndex();
    void replaceWorst(const Individual &child);

public:
    GeneticAlgorithm(const GAParameters &p, const ProblemData &data);

    Individual run();
    Individual runWithSeed(const vector<int> &seedChromosome);

    Individual getBestSolution() const { return bestSolution; }
    int getCurrentGeneration() const { return currentGeneration; }

    int getGenerationsExecuted() const { return static_cast<int>(history.size()); }
    vector<GenerationStats> getHistory() const { return history; }
};

string selectionTypeToString(SelectionType type);
string crossoverTypeToString(CrossoverType type);
string mutationTypeToString(MutationType type);


#endif
