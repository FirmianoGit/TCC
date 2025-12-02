#include "scheduling_ga.h"
#include "AlgoritmoGenetico/genetic_algorithm.h"
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <numeric>
#include <algorithm>

namespace fs = std::filesystem;
using namespace std::chrono;

struct InstanceResult
{
    string instanceFile;
    string permutationFile;
    int nJobs;
    int nStages;

    // Métricas básicas
    double initialFitness;
    double finalFitness;

    // Métricas expandidas
    double bestFitness;
    double worstFitness;
    double avgFitness;
    double stdDev;

    // Qualidade
    double improvement;
    double rpd;

    // Tempo
    double executionTimeMs;
    double timePerGenMs;

    // Convergência
    int populationSize;
    int generations;
    int convergenceGen;
    double convergencePercent;

    // Diversidade
    double fitnessDiversity;

    // Solução
    string bestChromosome;
    string gaConfig;
};

// Calcular métricas expandidas
void calculateExpandedMetrics(InstanceResult &result,
                              const Individual &bestSolution,
                              const vector<GenerationStats> &history,
                              double executionTimeMs,
                              int populationSize,
                              int generations,
                              double optimalKnown = -1.0)
{

    result.executionTimeMs = executionTimeMs;
    result.populationSize = populationSize;
    result.generations = generations;
    result.timePerGenMs = (generations > 0) ? executionTimeMs / generations : 0.0;

    // Fitness
    result.finalFitness = bestSolution.fitness;
    result.bestFitness = bestSolution.fitness;

    // Obter estatísticas do histórico (população final aproximada)
    if (!history.empty())
    {
        result.worstFitness = history.back().worstFitness;
        result.avgFitness = history.back().avgFitness;

        // Calcular StdDev (aproximado do histórico)
        double sumSqDiff = 0.0;
        for (const auto &gen : history)
        {
            sumSqDiff += pow(gen.bestFitness - result.avgFitness, 2);
        }
        result.stdDev = sqrt(sumSqDiff / history.size());
    }
    else
    {
        result.worstFitness = result.finalFitness;
        result.avgFitness = result.finalFitness;
        result.stdDev = 0.0;
    }

    // Melhoria
    result.improvement = (result.initialFitness > 0)
                             ? ((result.initialFitness - result.finalFitness) / result.initialFitness) * 100.0
                             : 0.0;

    // RPD (Relative Percentage Deviation)
    result.rpd = (optimalKnown > 0)
                     ? ((result.finalFitness - optimalKnown) / optimalKnown) * 100.0
                     : 0.0;

    // Convergência
    result.convergenceGen = 0;
    double threshold = 0.01;
    for (size_t i = 1; i < history.size(); ++i)
    {
        double change = abs(history[i].bestFitness - history[i - 1].bestFitness);
        if (change > threshold)
        {
            result.convergenceGen = i;
        }
    }
    result.convergencePercent = (generations > 0)
                                    ? (result.convergenceGen / (double)generations) * 100.0
                                    : 0.0;

    // Diversidade
    result.fitnessDiversity = (result.avgFitness > 0 && result.worstFitness > 0)
                                  ? (result.worstFitness - result.bestFitness) / result.avgFitness
                                  : 0.0;
}

void saveGenerationHistory(const string &outputDir, const string &instanceName,
                           const vector<GenerationStats> &history, const GAParameters &params)
{
    fs::create_directories(outputDir);

    stringstream filename;
    filename << outputDir << "\\generations_" << instanceName << ".csv";

    ofstream file(filename.str());
    if (!file.is_open())
    {
        cerr << "ERRO: Nao foi possivel criar arquivo de historico!" << endl;
        return;
    }

    file << "Generation,BestFitness,AvgFitness,WorstFitness,ElapsedTime\n";

    for (const auto &gen : history)
    {
        file << gen.generation << ","
             << fixed << setprecision(4) << gen.bestFitness << ","
             << gen.avgFitness << ","
             << gen.worstFitness << ","
             << gen.elapsedTime << "\n";
    }

    file.close();
}

void saveSummaryResults(const string &outputDir, const vector<InstanceResult> &results)
{
    fs::create_directories(outputDir);

    auto now = system_clock::now();
    auto timestamp = system_clock::to_time_t(now);

    stringstream filename;
    filename << outputDir << "\\summary_GA_" << timestamp << ".csv";

    ofstream file(filename.str());
    if (!file.is_open())
    {
        cerr << "ERRO: Nao foi possivel criar arquivo de resumo!" << endl;
        return;
    }

    // Novo cabeçalho expandido
    file << "Instance,Permutation,Jobs,Stages,InitialFitness,BestFitness,WorstFitness,"
         << "AvgFitness,StdDev,FinalFitness,Improvement(%),RPD(%),ExecutionTime_ms,"
         << "TimePerGen_ms,PopSize,Generations,ConvergenceGen,ConvergencePercent(%),"
         << "FitnessDiversity,BestChromosome,GAConfig\n";

    for (const auto &result : results)
    {
        file << result.instanceFile << ","
             << result.permutationFile << ","
             << result.nJobs << ","
             << result.nStages << ","
             << fixed << setprecision(4)
             << result.initialFitness << ","
             << result.bestFitness << ","
             << result.worstFitness << ","
             << result.avgFitness << ","
             << result.stdDev << ","
             << result.finalFitness << ","
             << result.improvement << ","
             << result.rpd << ","
             << result.executionTimeMs << ","
             << result.timePerGenMs << ","
             << result.populationSize << ","
             << result.generations << ","
             << result.convergenceGen << ","
             << result.convergencePercent << ","
             << result.fitnessDiversity << ","
             << result.bestChromosome << ","
             << result.gaConfig << "\n";
    }

    file.close();
    cout << "\nResumo salvo em: " << filename.str() << endl;
}

void printUsage(const char *programName)
{
    cout << "\n==================================================================" << endl;
    cout << "USO: " << programName << " [opcoes]" << endl;
    cout << "==================================================================" << endl;
    cout << "\nOPCOES:" << endl;
    cout << "  --instances <dir>     Diretorio das instancias" << endl;
    cout << "  --permutations <dir>  Diretorio das permutacoes iniciais" << endl;
    cout << "  --output <dir>        Diretorio de saida" << endl;
    cout << "  --duedate <valor>     Due date padrao" << endl;
    cout << "\nOPCOES DO GA:" << endl;
    cout << "  --selection <tipo>    tournament | roulette" << endl;
    cout << "  --crossover <tipo>    obx | pmx | sb2ox | opx | tpx" << endl;
    cout << "  --mutation <tipo>     insert | interchange | swap" << endl;
    cout << "  --popsize <n>         30 | 70 | 110 | 150" << endl;
    cout << "  --pc <valor>          0.8 | 0.95 | 1.0" << endl;
    cout << "  --pm <valor>          0.00 | 0.03 | 0.05" << endl;
    cout << "  --restart <gens>      30 | 50 | inf" << endl;
    cout << "  --lsfreq <gens>       5 | 10 | inf" << endl;
    cout << "  --lsintensity <f>     1 | 5" << endl;
    cout << "  --time <segundos>     Tempo maximo por instancia" << endl;
    cout << "\nEXEMPLO:" << endl;
    cout << "  " << programName << " --instances ./Instancias --permutations ./Permutacoes --output ./Resultados" << endl;
    cout << "==================================================================\n"
         << endl;
}

int main(int argc, char *argv[])
{
    string instancesDir = R"(C:\Users\Firmiano\Desktop\TCC\Implementacao_com_codificacao\Instancias)";
    string permutationsDir = R"(C:\Users\Firmiano\Desktop\TCC\Implementacao_com_codificacao\Permutacoes)";
    string outputDir = R"(C:\Users\Firmiano\Desktop\TCC\Implementacao_com_codificacao\Resultados)";
    int defaultDueDate = 100;

    GAParameters gaParams;

    for (int i = 1; i < argc; i++)
    {
        string arg = argv[i];

        if (arg == "--instances" && i + 1 < argc)
        {
            instancesDir = argv[++i];
        }
        else if (arg == "--permutations" && i + 1 < argc)
        {
            permutationsDir = argv[++i];
        }
        else if (arg == "--output" && i + 1 < argc)
        {
            outputDir = argv[++i];
        }
        else if (arg == "--duedate" && i + 1 < argc)
        {
            defaultDueDate = stoi(argv[++i]);
        }
        else if (arg == "--selection" && i + 1 < argc)
        {
            string value = argv[++i];
            if (value == "tournament")
                gaParams.selectionType = SelectionType::TOURNAMENT;
            else if (value == "roulette")
                gaParams.selectionType = SelectionType::ROULETTE_WHEEL;
        }
        else if (arg == "--crossover" && i + 1 < argc)
        {
            string value = argv[++i];
            if (value == "obx")
                gaParams.crossoverType = CrossoverType::OBX;
            else if (value == "pmx")
                gaParams.crossoverType = CrossoverType::PMX;
            else if (value == "sb2ox")
                gaParams.crossoverType = CrossoverType::SB2OX;
            else if (value == "opx")
                gaParams.crossoverType = CrossoverType::OPX;
            else if (value == "tpx")
                gaParams.crossoverType = CrossoverType::TPX;
        }
        else if (arg == "--mutation" && i + 1 < argc)
        {
            string value = argv[++i];
            if (value == "insert")
                gaParams.mutationType = MutationType::INSERT;
            else if (value == "interchange")
                gaParams.mutationType = MutationType::INTERCHANGE;
            else if (value == "swap")
                gaParams.mutationType = MutationType::SWAP;
        }
        else if (arg == "--popsize" && i + 1 < argc)
        {
            gaParams.populationSize = stoi(argv[++i]);
        }
        else if (arg == "--pc" && i + 1 < argc)
        {
            gaParams.crossoverProb = stod(argv[++i]);
        }
        else if (arg == "--pm" && i + 1 < argc)
        {
            gaParams.mutationProb = stod(argv[++i]);
        }
        else if (arg == "--restart" && i + 1 < argc)
        {
            string value = argv[++i];
            if (value == "inf")
                gaParams.restartGenerations = INT_MAX;
            else
                gaParams.restartGenerations = stoi(value);
        }
        else if (arg == "--lsfreq" && i + 1 < argc)
        {
            string value = argv[++i];
            if (value == "inf")
                gaParams.localSearchFreq = INT_MAX;
            else
                gaParams.localSearchFreq = stoi(value);
        }
        else if (arg == "--lsintensity" && i + 1 < argc)
        {
            gaParams.localSearchIntensity = stoi(argv[++i]);
        }
        else if (arg == "--time" && i + 1 < argc)
        {
            gaParams.maxCPUTimeSeconds = stod(argv[++i]);
        }
        else if (arg == "--help" || arg == "-h")
        {
            printUsage(argv[0]);
            return 0;
        }
    }

    cout << "============================================================" << endl;
    cout << "HYBRID FLOWSHOP - ALGORITMO GENETICO COM SEED" << endl;
    cout << "============================================================" << endl;
    cout << "Instancias:   " << instancesDir << endl;
    cout << "Permutacoes:  " << permutationsDir << endl;
    cout << "Resultados:   " << outputDir << endl;
    cout << "Due Date:     " << defaultDueDate << endl;
    cout << "============================================================\n"
         << endl;

    vector<string> instanceFiles;
    for (const auto &entry : fs::directory_iterator(instancesDir))
    {
        if (entry.path().extension() == ".txt")
        {
            instanceFiles.push_back(entry.path().filename().string());
        }
    }

    sort(instanceFiles.begin(), instanceFiles.end(), [](const string &a, const string &b)
         {
        int numA = stoi(a.substr(1, a.find('.') - 1));
        int numB = stoi(b.substr(1, b.find('.') - 1));
        return numA < numB; });

    vector<InstanceResult> results;
    int total = instanceFiles.size();
    int processed = 0;

    for (const auto &instanceFile : instanceFiles)
    {
        processed++;

        int instanceId = stoi(instanceFile.substr(1, instanceFile.find('.') - 1));
        fs::path instancePath = fs::path(instancesDir) / instanceFile;
        string permutationFile = "P" + to_string(instanceId) + ".txt";
        fs::path permutationPath = fs::path(permutationsDir) / permutationFile;

        cout << "\n[" << processed << "/" << total << "] Processando " << instanceFile << endl;
        cout << "-------------------------------------------------------------" << endl;

        if (!fs::exists(permutationPath))
        {
            cout << "AVISO: Permutacao " << permutationFile << " nao encontrada! Pulando..." << endl;
            continue;
        }

        ProblemData problem;
        if (!readInstanceFromFile(instancePath.string(), problem, defaultDueDate))
        {
            cout << "ERRO ao ler instancia!" << endl;
            continue;
        }

        vector<int> seedPermutation;
        if (!readPermutationFromFile(permutationPath.string(), seedPermutation))
        {
            cout << "ERRO ao ler permutacao!" << endl;
            continue;
        }

        // Converter para 0-based
        vector<int> seedChromosome(seedPermutation.size());
        for (size_t i = 0; i < seedPermutation.size(); ++i)
        {
            seedChromosome[i] = seedPermutation[i] - 1;
        }

        // Calcular fitness inicial
        ProblemData dataCopy = problem;
        double initialFitness = decodeChromosome(seedPermutation, dataCopy);

        cout << "Fitness inicial (seed): " << fixed << setprecision(2) << initialFitness << endl;

        // Executar GA
        auto start = high_resolution_clock::now();

        GeneticAlgorithm ga(gaParams, problem);
        Individual bestSolution = ga.runWithSeed(seedChromosome);

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        // ✅ Obter número real de gerações
        int actualGenerations = ga.getGenerationsExecuted();


        // Salvar histórico de gerações
        string instanceName = instanceFile.substr(0, instanceFile.find('.'));
        saveGenerationHistory(outputDir, instanceName, ga.getHistory(), gaParams);

        // Formatar cromossomo como string
        stringstream chromosomeStr;
        chromosomeStr << "[";
        for (size_t i = 0; i < bestSolution.chromosome.size(); ++i)
        {
            chromosomeStr << bestSolution.chromosome[i];
            if (i < bestSolution.chromosome.size() - 1)
                chromosomeStr << " ";
        }
        chromosomeStr << "]";

        // Formatar configuração do GA
        stringstream configStr;
        configStr << selectionTypeToString(gaParams.selectionType) << "|"
                  << crossoverTypeToString(gaParams.crossoverType) << "|"
                  << mutationTypeToString(gaParams.mutationType) << "|"
                  << "Pop:" << gaParams.populationSize << "|"
                  << "Pc:" << gaParams.crossoverProb << "|"
                  << "Pm:" << gaParams.mutationProb;

        InstanceResult result;
        result.instanceFile = instanceFile;
        result.permutationFile = permutationFile;
        result.nJobs = problem.numJobs;
        result.nStages = problem.numStages;
        result.initialFitness = initialFitness;
        result.bestChromosome = chromosomeStr.str();
        result.gaConfig = configStr.str();

        // Calcular métricas expandidas
        calculateExpandedMetrics(result, bestSolution, ga.getHistory(),
                                 duration.count(), gaParams.populationSize,
                                 actualGenerations);

        results.push_back(result);

        cout << "\nResultado:" << endl;
        cout << "  Fitness inicial: " << result.initialFitness << endl;
        cout << "  Fitness final:   " << result.finalFitness << endl;
        cout << "  Melhoria:        " << fixed << setprecision(2) << result.improvement << "%" << endl;
        cout << "  Tempo:           " << result.executionTimeMs << " ms" << endl;
        cout << "  Convergencia:    " << result.convergenceGen << " geracoes" << endl;
        cout << "  ✅ Gerações:      " << actualGenerations << endl;  
        cout << "-------------------------------------------------------------" << endl;
    }

    saveSummaryResults(outputDir, results);

    cout << "\n============================================================" << endl;
    cout << "PROCESSAMENTO CONCLUIDO" << endl;
    cout << "============================================================" << endl;
    cout << "Total de instancias processadas: " << results.size() << endl;
    cout << "Arquivos gerados em: " << outputDir << endl;
    cout << "  - summary_GA_<timestamp>.csv: Resumo geral (EXPANDIDO)" << endl;
    cout << "  - generations_<instance>.csv: Historico por geracao" << endl;
    cout << "============================================================\n"
         << endl;

    return 0;
}