#include "ModeloProblema.h"

// NOVA FUNÇÃO: Ler instância do arquivo gerado pelo Python
bool readInstanceFromFile(const string& filename, ProblemData& data, int defaultDueDate) {
    ifstream file(filename.c_str());
    if (!file.is_open()) {
        cerr << "Erro: nao foi possível abrir " << filename << endl;
        return false;
    }

    // Ler header: n_jobs n_stages
    file >> data.numJobs >> data.numStages;

    // Ler número de máquinas por estágio
    data.machinesPerStage.resize(data.numStages);
    for (int i = 0; i < data.numStages; i++) {
        file >> data.machinesPerStage[i];
    }

    // Inicializar vetor de jobs
    data.jobs.clear();
    data.jobs.resize(data.numJobs);

    // Ler tempos de processamento por estágio
    string line;
    getline(file, line); // Consumir newline após header

    for (int stage = 0; stage < data.numStages; stage++) {
        // Ler linha de comentário (# Stage X)
        getline(file, line);

        // Ler tempos para todos os jobs neste estágio
        for (int job = 0; job < data.numJobs; job++) {
            // Inicializar job na primeira vez
            if (stage == 0) {
                data.jobs[job].id = job + 1; // IDs começam em 1
                data.jobs[job].dueDate = defaultDueDate; // Due date padrão
                data.jobs[job].priority = 0.0;
                data.jobs[job].tardiness = 0.0;
                data.jobs[job].processingTimes.resize(data.numStages);
                data.jobs[job].completionTimes.resize(data.numStages, 0.0);
            }

            // Redimensionar vetor de máquinas para este estágio
            data.jobs[job].processingTimes[stage].resize(data.machinesPerStage[stage]);

            // Ler tempos de processamento para cada máquina
            for (int machine = 0; machine < data.machinesPerStage[stage]; machine++) {
                file >> data.jobs[job].processingTimes[stage][machine];
            }
        }
    }

    file.close();

    // Inicializar máquinas
    data.machines.clear();
    for (int i = 1; i <= data.numStages; ++i) {
        for (int l = 1; l <= data.machinesPerStage[i - 1]; ++l) {
            data.machines.emplace(make_pair(i, l), Machine(i, l));
        }
    }

    return true;
}

// NOVA FUNÇÃO: Ler permutação do arquivo Python
bool readPermutationFromFile(const string& filename, vector<int>& permutation) {
    ifstream file(filename.c_str());
    if (!file.is_open()) {
        cerr << "Erro: nao foi possível abrir " << filename << endl;
        return false;
    }

    permutation.clear();
    int job;
    while (file >> job) {
        permutation.push_back(job);
    }

    file.close();

    // Converter de índice 0-based para 1-based (se necessário)
    // O gerador Python usa índices 0-based (0, 1, 2, ...)
    // Mas o código usa IDs 1-based (1, 2, 3, ...)
    for (int& jobId : permutation) {
        jobId += 1; // Converter para 1-based
    }

    return true;
}

// FUNÇÃO ALTERADA: Agora considera múltiplas máquinas por estágio
int getProcessingTime(Job* job, int stageId, int machineId) {
    // stageId é 1-baseado, o índice do vetor é 0-baseado
    // machineId é 1-baseado, o índice do vetor é 0-baseado
    if (stageId > 0 && stageId <= (int)job->processingTimes.size()) {
        int stageIdx = stageId - 1;
        int machineIdx = machineId - 1;
        if (machineIdx >= 0 && machineIdx < (int)job->processingTimes[stageIdx].size()) {
            return job->processingTimes[stageIdx][machineIdx];
        }
    }
    return 0;
}

// 1. Job_assign(job, stageId) -> Machine
Machine* Job_assign(Job* job, int stageId, ProblemData& data, double systemClock) {
    double minWorkload = numeric_limits<double>::max();
    Machine* selectedMachine = nullptr;

    // Iterar sobre todas as máquinas no estágio
    for (int l = 1; l <= data.machinesPerStage[stageId - 1]; ++l) {
        Machine& machine = data.getMachine(stageId, l);

        // 1. Soma dos tempos de processamento dos jobs já no buffer
        double bufferProcessingTime = 0.0;
        for (Job* bufferedJob : machine.buffer) {
            bufferProcessingTime += getProcessingTime(bufferedJob, stageId, l);
        }

        // 2. Tempo de processamento do job atual nesta máquina
        double currentJobProcessingTime = getProcessingTime(job, stageId, l);

        // 3. Tempo restante antes da máquina estar disponível
        double remainingIdleTime = max(0.0, machine.availableTime - systemClock);

        // Workload esperado
        double expectedWorkload = bufferProcessingTime + currentJobProcessingTime + remainingIdleTime;

        // Regra de seleção: argmin do workload esperado
        if (expectedWorkload < minWorkload) {
            minWorkload = expectedWorkload;
            selectedMachine = &machine;
        }
    }

    if (selectedMachine) {
        selectedMachine->addToBuffer(job);
    }

    return selectedMachine;
}

// 2. Machine_seize(machine) -> job, completionTime
pair<Job*, double> Machine_seize(Machine* machine, double systemClock) {
    if (machine->buffer.empty()) {
        return {nullptr, 0.0};
    }

    // Encontrar o job com a maior prioridade (menor valor de rho_j)
    auto it = min_element(machine->buffer.begin(), machine->buffer.end(),
        [](Job* a, Job* b) {
            return a->priority < b->priority;
        });

    Job* seizedJob = *it;
    machine->buffer.erase(it);

    int stageIdx = machine->stageId - 1;
    double processingTime = getProcessingTime(seizedJob, machine->stageId, machine->machineId);
    double completionTime = systemClock + processingTime;

    // Atualizar o estado da máquina e do job
    machine->availableTime = completionTime;
    machine->isBusy = 1;
    machine->currentJob = seizedJob;
    seizedJob->completionTimes[stageIdx] = completionTime;

    return {seizedJob, completionTime};
}

// 3. Machine_release(machine) -> job
Job* Machine_release(Machine* machine, double systemClock) {
    Job* releasedJob = machine->currentJob;

    machine->isBusy = 0;
    machine->availableTime = systemClock;
    machine->currentJob = nullptr;

    return releasedJob;
}

// Função de comparação para a priority_queue de Eventos
struct CompareEvent {
    bool operator()(const Event& a, const Event& b) {
        if (a.time != b.time) {
            return a.time > b.time;
        }
        return a.stageId > b.stageId;
    }
};

// Função principal de decodificação (Algoritmo 1)
double decodeChromosome(const vector<int>& chromosome, ProblemData& data) {
    priority_queue<Event, vector<Event>, CompareEvent> eventList;

    double systemClock = 0.0;
    int numJobsCompleted = 0;

    // 1. Inicialização - Definir prioridades baseadas no cromossomo
    for (size_t k = 0; k < chromosome.size(); ++k) {
        int jobId = chromosome[k]; // Job na posição k (1-based)
        data.jobs[jobId - 1].priority = (double)k + 1.0;
    }

    // 2. Loop de Atribuição Inicial - Atribui jobs ao primeiro estágio
    for (size_t k = 0; k < chromosome.size(); ++k) {
        int jobId = chromosome[k];
        Job* job = &data.jobs[jobId - 1];
        int stageId = 1;

        Machine* selectedMachine = Job_assign(job, stageId, data, systemClock);

        if (selectedMachine && selectedMachine->isBusy == 0) {
            pair<Job*, double> result = Machine_seize(selectedMachine, systemClock);
            double completionTime = result.second;

            Event newEvent = {completionTime, selectedMachine->stageId, selectedMachine->machineId};
            eventList.push(newEvent);
        }
    }

    // 3. Loop Principal
    while (numJobsCompleted < data.numJobs) {
        if (eventList.empty()) {
            break;
        }

        Event currentEvent = eventList.top();
        eventList.pop();

        systemClock = currentEvent.time;

        Machine& machine = data.getMachine(currentEvent.stageId, currentEvent.machineId);

        Job* releasedJob = Machine_release(&machine, systemClock);
        if (!releasedJob) continue;

        int currentStageId = machine.stageId;
        int nextStageId = currentStageId + 1;

        if (currentStageId < data.numStages) {
            Machine* nextMachine = Job_assign(releasedJob, nextStageId, data, systemClock);

            if (nextMachine && nextMachine->isBusy == 0) {
                pair<Job*, double> result = Machine_seize(nextMachine, systemClock);
                double completionTime = result.second;

                Event newEvent = {completionTime, nextMachine->stageId, nextMachine->machineId};
                eventList.push(newEvent);
            }
        } else {
            numJobsCompleted++;
        }

        if (!machine.buffer.empty()) {
            pair<Job*, double> result = Machine_seize(&machine, systemClock);
            double completionTime = result.second;

            Event newEvent = {completionTime, machine.stageId, machine.machineId};
            eventList.push(newEvent);
        }
    }

    // 4. Cálculo Final
    double totalTardiness = 0.0;
    for (Job& job : data.jobs) {
        double finalCompletionTime = job.completionTimes[data.numStages - 1];
        job.tardiness = max(0.0, finalCompletionTime - job.dueDate);
        totalTardiness += job.tardiness;
    }

    return totalTardiness;
}
