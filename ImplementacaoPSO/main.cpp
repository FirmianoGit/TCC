#include "AlgoritmoPSO/scheduling_pso.h"
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <cmath>
#include <numeric>

namespace fs = std::filesystem;
using namespace std;
using namespace std::chrono;

struct InstanceResult {
    string instanceName;
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
    string bestPosition;
    string psoConfig;
};

// Calcular métricas expandidas
void calculateExpandedMetrics(InstanceResult& result,
                              const vector<GenerationStats>& history,
                              double executionTimeMs,
                              int populationSize,
                              int generations,
                              double optimalKnown = -1.0) {

    result.executionTimeMs = executionTimeMs;
    result.populationSize = populationSize;
    result.generations = generations;
    result.timePerGenMs = (generations > 0) ? executionTimeMs / generations : 0.0;

    if (history.empty()) {
        result.worstFitness = result.finalFitness;
        result.avgFitness = result.finalFitness;
        result.stdDev = 0.0;
        result.convergenceGen = 0;
        result.convergencePercent = 0.0;
        result.fitnessDiversity = 0.0;
        return;
    }

    // Fitness
    result.bestFitness = history.back().bestFitness;
    result.finalFitness = history.back().bestFitness;
    result.worstFitness = history.back().worstFitness;
    result.avgFitness = history.back().avgFitness;

    // Calcular StdDev do histórico
    double sumSqDiff = 0.0;
    for (const auto& gen : history) {
        sumSqDiff += pow(gen.bestFitness - result.avgFitness, 2);
    }
    result.stdDev = sqrt(sumSqDiff / history.size());

    // Melhoria
    result.improvement = (result.initialFitness > 0)
        ? ((result.initialFitness - result.finalFitness) / result.initialFitness) * 100.0
        : 0.0;

    // RPD
    result.rpd = (optimalKnown > 0)
        ? ((result.finalFitness - optimalKnown) / optimalKnown) * 100.0
        : 0.0;

    // Convergência - encontrar última mudança significativa
    result.convergenceGen = 0;
    double threshold = 0.01;
    for (size_t i = 1; i < history.size(); ++i) {
        double change = abs(history[i].bestFitness - history[i-1].bestFitness);
        if (change > threshold) {
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

int main(int argc, char* argv[]) {
    cout << "============================================================" << endl;
    cout << "  PSO - PROCESSAMENTO EM LOTE (BATCH)" << endl;
    cout << "  Hybrid Flowshop Scheduling" << endl;
    cout << "============================================================" << endl << endl;

    // Parâmetros padrão do PSO
    int populationSize = 100;
    int numGenerations = 500;
    double c1 = 0.2;
    double c2 = 0.2;
    double inertiaWeight = 0.5;
    double mutationProb = 0.9;
    int crossoverType = 4;        // PTL
    int mutationOperator = 4;     // Multiple Insert

    // Diretórios
    string instancesDir = "./Instancias";
    string outputDir = "./Resultados";

    // Processar argumentos
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--instances" && i + 1 < argc) {
            instancesDir = argv[++i];
        }
        else if (arg == "--output" && i + 1 < argc) {
            outputDir = argv[++i];
        }
        else if (arg == "--popsize" && i + 1 < argc) {
            populationSize = stoi(argv[++i]);
        }
        else if (arg == "--generations" && i + 1 < argc) {
            numGenerations = stoi(argv[++i]);
        }
        else if (arg == "--c1" && i + 1 < argc) {
            c1 = stod(argv[++i]);
        }
        else if (arg == "--c2" && i + 1 < argc) {
            c2 = stod(argv[++i]);
        }
        else if (arg == "--mutation" && i + 1 < argc) {
            mutationProb = stod(argv[++i]);
        }
        else if (arg == "--crossover" && i + 1 < argc) {
            crossoverType = stoi(argv[++i]);
        }
        else if (arg == "--mutoperator" && i + 1 < argc) {
            mutationOperator = stoi(argv[++i]);
        }
        else if (arg == "--help" || arg == "-h") {
            cout << "USO: " << argv[0] << " [opcoes]" << endl;
            cout << "\nOPCOES:" << endl;
            cout << "  --instances <dir>     Diretorio das instancias (padrao: ./Instancias)" << endl;
            cout << "  --output <dir>        Diretorio de saida (padrao: ./Resultados)" << endl;
            cout << "  --popsize <n>         Tamanho populacao (padrao: 100)" << endl;
            cout << "  --generations <n>     Numero de geracoes (padrao: 500)" << endl;
            cout << "  --c1 <valor>          Coef. local best (padrao: 0.2)" << endl;
            cout << "  --c2 <valor>          Coef. global best (padrao: 0.2)" << endl;
            cout << "  --mutation <valor>    Prob. mutacao (padrao: 0.9)" << endl;
            cout << "  --crossover <tipo>    1=OC, 2=TP, 3=PMX, 4=PTL (padrao: 4)" << endl;
            cout << "  --mutoperator <tipo>  1=Swap, 2=Insert, 3=MS, 4=MI (padrao: 4)" << endl;
            cout << "\nEXEMPLO:" << endl;
            cout << "  " << argv[0] << " --instances ./Instancias --output ./Resultados" << endl;
            return 0;
        }
    }

    // Mostrar configuração
    cout << "CONFIGURACAO:" << endl;
    cout << "  Instancias:   " << instancesDir << endl;
    cout << "  Resultados:   " << outputDir << endl;
    cout << "  Populacao:    " << populationSize << endl;
    cout << "  Geracoes:     " << numGenerations << endl;
    cout << "  c1:           " << c1 << endl;
    cout << "  c2:           " << c2 << endl;
    cout << "  Mutacao prob: " << mutationProb << endl;
    cout << "  Crossover:    " << crossoverType;
    if (crossoverType == 4) cout << " (PTL)";
    cout << endl;
    cout << "  Mutacao op:   " << mutationOperator;
    if (mutationOperator == 4) cout << " (MultiInsert)";
    cout << endl;
    cout << "============================================================" << endl << endl;

    // Criar diretório de saída
    fs::create_directories(outputDir);

    // Listar todos os arquivos I*.txt
    vector<string> instanceFiles;
    try {
        for (const auto& entry : fs::directory_iterator(instancesDir)) {
            if (entry.is_regular_file()) {
                string filename = entry.path().filename().string();
                if (filename[0] == 'I' && entry.path().extension() == ".txt") {
                    instanceFiles.push_back(filename);
                }
            }
        }
    } catch (const exception& e) {
        cerr << "ERRO ao ler diretorio de instancias: " << e.what() << endl;
        return 1;
    }

    if (instanceFiles.empty()) {
        cerr << "ERRO: Nenhum arquivo I*.txt encontrado em " << instancesDir << endl;
        return 1;
    }

    // Ordenar por número (I1, I2, ..., I10, ...)
    sort(instanceFiles.begin(), instanceFiles.end(), [](const string& a, const string& b) {
        int numA = stoi(a.substr(1, a.find('.') - 1));
        int numB = stoi(b.substr(1, b.find('.') - 1));
        return numA < numB;
    });

    cout << "Encontradas " << instanceFiles.size() << " instancias para processar." << endl;
    cout << "============================================================" << endl << endl;

    // Processar cada instância
    vector<InstanceResult> results;
    int processed = 0;
    int total = instanceFiles.size();

    for (const auto& instanceFile : instanceFiles) {
        processed++;

        string instancePath = instancesDir + "/" + instanceFile;
        string instanceName = instanceFile.substr(0, instanceFile.find('.'));
        string outputFile = outputDir + "/generations_" + instanceName + ".csv";

        cout << "[" << processed << "/" << total << "] Processando " << instanceFile << endl;
        cout << "-------------------------------------------------------------" << endl;

        // Criar PSO
        PSO pso(populationSize, numGenerations, c1, c2, inertiaWeight,
                mutationProb, crossoverType, mutationOperator);

        // Medir tempo
        auto startTime = high_resolution_clock::now();

        // Executar PSO
        pso.run(instancePath, outputFile);

        auto endTime = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(endTime - startTime);

        // Obter histórico
        const auto& history = pso.getHistory();

        if (history.empty()) {
            cout << "AVISO: Nenhum historico gerado!" << endl;
            continue;
        }

        // Ler dados da instância
        int nJobs = 0, nStages = 0;
        ifstream instFile(instancePath);
        if (instFile.is_open()) {
            instFile >> nJobs >> nStages;
            instFile.close();
        }

        // Salvar resultado
        InstanceResult result;
        result.instanceName = instanceName;
        result.nJobs = nJobs;
        result.nStages = nStages;
        result.initialFitness = history[0].bestFitness;
        result.finalFitness = history.back().bestFitness;

        // Obter melhor posição (assumindo que PSO.h tem método getGlobalBest())
        // Se não tiver, usar placeholder
        result.bestPosition = pso.getGlobalBestPositionString();

        result.psoConfig = "PSO|Pop:" + to_string(populationSize) + "|Gen:" +
                          to_string(numGenerations) + "|c1:" + to_string(c1) +
                          "|c2:" + to_string(c2) + "|Pm:" + to_string(mutationProb) +
                          "|Cross:" + to_string(crossoverType) + "|Mut:" + to_string(mutationOperator);

        // Calcular métricas expandidas
        calculateExpandedMetrics(result, history, duration.count(),
                                populationSize, numGenerations);

        results.push_back(result);

        // Mostrar resultado
        cout << "  Jobs x Stages:   " << nJobs << " x " << nStages << endl;
        cout << "  Fitness inicial: " << fixed << setprecision(2) << result.initialFitness << endl;
        cout << "  Fitness final:   " << result.finalFitness << endl;
        cout << "  Melhoria:        " << result.improvement << "%" << endl;
        cout << "  StdDev:          " << result.stdDev << endl;
        cout << "  Convergencia:    " << result.convergenceGen << " geracoes ("
             << result.convergencePercent << "%)" << endl;
        cout << "  Tempo:           " << result.executionTimeMs << " ms" << endl;
        cout << "-------------------------------------------------------------" << endl << endl;
    }

    // Salvar resumo geral
    auto now = system_clock::now();
    auto timestamp = system_clock::to_time_t(now);

    stringstream summaryFilename;
    summaryFilename << outputDir << "/summary_PSO_" << timestamp << ".csv";

    ofstream summaryFile(summaryFilename.str());
    if (summaryFile.is_open()) {
        // Novo cabeçalho expandido
        summaryFile << "Instance,Jobs,Stages,InitialFitness,BestFitness,WorstFitness,"
                   << "AvgFitness,StdDev,FinalFitness,Improvement(%),RPD(%),"
                   << "ExecutionTime_ms,TimePerGen_ms,PopSize,Generations,ConvergenceGen,"
                   << "ConvergencePercent(%),FitnessDiversity,BestChromosome,PSOConfig\n";

        // Dados
        for (const auto& result : results) {
            summaryFile << result.instanceName << ","
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
                       << result.bestPosition << ","
                       << result.psoConfig << "\n";
        }

        summaryFile.close();
        cout << "Resumo salvo em: " << summaryFilename.str() << endl;
    }

    // Estatísticas finais
    cout << "\n============================================================" << endl;
    cout << "PROCESSAMENTO CONCLUIDO" << endl;
    cout << "============================================================" << endl;
    cout << "Total de instancias: " << results.size() << endl;

    if (!results.empty()) {
        double avgImprovement = 0.0;
        double avgTime = 0.0;
        double avgConvergence = 0.0;
        double avgStdDev = 0.0;

        for (const auto& r : results) {
            avgImprovement += r.improvement;
            avgTime += r.executionTimeMs;
            avgConvergence += r.convergenceGen;
            avgStdDev += r.stdDev;
        }

        avgImprovement /= results.size();
        avgTime /= results.size();
        avgConvergence /= results.size();
        avgStdDev /= results.size();

        cout << "\nESTATISTICAS GERAIS:" << endl;
        cout << "  Melhoria media:      " << fixed << setprecision(2) << avgImprovement << "%" << endl;
        cout << "  Tempo medio:         " << avgTime << " ms" << endl;
        cout << "  Convergencia media:  " << (int)avgConvergence << " geracoes" << endl;
        cout << "  StdDev medio:        " << avgStdDev << endl;
    }

    cout << "\nArquivos gerados:" << endl;
    cout << "  - generations_<instance>.csv  (um por instancia)" << endl;
    cout << "  - summary_PSO_<timestamp>.csv (resumo geral EXPANDIDO)" << endl;
    cout << "\nDiretorio: " << outputDir << endl;
    cout << "============================================================" << endl;

    return 0;
}