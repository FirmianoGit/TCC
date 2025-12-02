#ifndef SCHEDULING_GA_H
#define SCHEDULING_GA_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <queue>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <limits>

using namespace std;

// --- Estruturas de Dados ---

// Estrutura para representar um Job
struct Job {
    int id;                                // Identificador do Job (j)
    int dueDate;                           // Tempo de entrega (d_j)
    double priority;                       // Prioridade (rho_j) derivada do cromossomo
    vector<vector<int>> processingTimes;   // ALTERADO: processingTimes[stage][machine]
    vector<double> completionTimes;        // Tempos de término por estágio (C_ij)
    double tardiness;                      // Tardiness (T_j)

    Job() : id(0), dueDate(0), priority(0.0), tardiness(0.0) {}

    Job(int j, int d, const vector<vector<int>>& p)
        : id(j), dueDate(d), processingTimes(p), priority(0.0), tardiness(0.0) {
        completionTimes.resize(p.size(), 0.0);
    }

    // Sobrecarga para comparação de prioridade (usada para selecionar job do buffer)
    bool operator<(const Job& other) const {
        return priority < other.priority; // Maior prioridade (menor rho) vem primeiro
    }
};

// Estrutura para representar um Evento (E = {t, M_il})
struct Event {
    double time;                           // Tempo de ocorrência (t)
    int stageId;                           // ID do Estágio (i)
    int machineId;                         // ID da Máquina (l)

    bool operator>(const Event& other) const {
        if (time != other.time) {
            return time > other.time;
        }
        return false;
    }
};

// Estrutura para representar uma Máquina (M_il)
struct Machine {
    int stageId;                           // ID do Estágio (i)
    int machineId;                         // ID da Máquina (l)
    double availableTime;                  // Tempo em que a máquina estará disponível (a_il)
    int isBusy;                            // 1 se ocupada, 0 se ociosa (b_il)
    vector<Job*> buffer;                   // Buffer de Jobs esperando (B_il)
    Job* currentJob;                       // Job atualmente em processamento

    Machine() : stageId(0), machineId(0), availableTime(0.0), isBusy(0), currentJob(nullptr) {}

    Machine(int i, int l) : stageId(i), machineId(l), availableTime(0.0), isBusy(0), currentJob(nullptr) {}

    void addToBuffer(Job* job) {
        buffer.push_back(job);
    }
};

// Estrutura para armazenar os parâmetros do problema
struct ProblemData {
    int numJobs;                           // n
    int numStages;                         // m
    vector<int> machinesPerStage;          // h_i (número de máquinas em cada estágio)
    vector<Job> jobs;                      // Lista de todos os Jobs
    map<pair<int, int>, Machine> machines; // Mapa de máquinas: <stageId, machineId> -> Machine

    ProblemData() : numJobs(0), numStages(0) {}

    ProblemData(int n, int m, const vector<int>& h) : numJobs(n), numStages(m), machinesPerStage(h) {}

    Machine& getMachine(int stageId, int machineId) {
        return machines.at({stageId, machineId});
    }
};

// --- NOVA FUNÇÃO: Ler instância do arquivo Python ---
bool readInstanceFromFile(const string& filename, ProblemData& data, int defaultDueDate = 100);

// --- NOVA FUNÇÃO: Ler permutação do arquivo Python ---
bool readPermutationFromFile(const string& filename, vector<int>& permutation);

// --- Funções do Algoritmo DS (Dynamic AlgoritmoPSO) ---
Machine* Job_assign(Job* job, int stageId, ProblemData& data, double systemClock);
pair<Job*, double> Machine_seize(Machine* machine, double systemClock);
Job* Machine_release(Machine* machine, double systemClock);
double decodeChromosome(const vector<int>& chromosome, ProblemData& data);

#endif // SCHEDULING_GA_H
