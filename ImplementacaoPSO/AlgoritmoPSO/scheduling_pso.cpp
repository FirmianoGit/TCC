#include "scheduling_pso.h"
#include <fstream>
#include <iomanip>

PSO::PSO(int popSize, int numGen, double c1_val, double c2_val, double inertia,
         double mutProb, int crossType, int mutType)
    : populationSize(popSize), numGenerations(numGen), c1(c1_val), c2(c2_val),
      inertiaWeight(inertia), mutationProb(mutProb), crossoverType(crossType),
      mutationOperator(mutType), uniformDist(0.0, 1.0) {
    random_device rd;
    rng.seed(rd());
}

PSO::~PSO() {}

void PSO::resetProblemData() {
    // Limpar máquinas
    for (auto& machine : problemData.machines) {
        machine.second.buffer.clear();
        machine.second.availableTime = 0.0;
        machine.second.isBusy = 0;
        machine.second.currentJob = nullptr;
    }

    // Limpar jobs
    for (auto& job : problemData.jobs) {
        job.priority = 0.0;
        job.tardiness = 0.0;
        fill(job.completionTimes.begin(), job.completionTimes.end(), 0.0);
    }
}

void PSO::initializeSwarm() {
    swarm.clear();
    swarm.resize(populationSize);

    // Criar uma permutação base
    vector<int> basePermutation(problemData.numJobs);
    for (int i = 0; i < problemData.numJobs; i++) {
        basePermutation[i] = i + 1;  // IDs começam em 1
    }

    for (int p = 0; p < populationSize; p++) {
        swarm[p].position = basePermutation;
        swarm[p].bestPosition = basePermutation;

        // Embaralhar para criar diversidade
        shuffle(swarm[p].position.begin(), swarm[p].position.end(), rng);

        // Inicializar velocidade como lista de movimentos
        swarm[p].velocity.resize(problemData.numJobs);
        for (int i = 0; i < problemData.numJobs; i++) {
            swarm[p].velocity[i] = i;
        }
        shuffle(swarm[p].velocity.begin(), swarm[p].velocity.end(), rng);

        // Avaliar partícula
        resetProblemData();
        swarm[p].fitness = evaluateParticle(swarm[p].position);
        swarm[p].bestFitness = swarm[p].fitness;
        swarm[p].bestPosition = swarm[p].position;

        // Atualizar global best
        if (swarm[p].fitness < globalBest.bestFitness) {
            globalBest.bestFitness = swarm[p].fitness;
            globalBest.bestPosition = swarm[p].position;
        }
    }
}

double PSO::evaluateParticle(vector<int>& position) {
    return decodeChromosome(position, problemData);
}

// ===== OPERADORES DE CROSSOVER =====

vector<int> PSO::orderCrossover(const vector<int>& parent1, const vector<int>& parent2) {
    int n = parent1.size();
    vector<int> offspring(n);
    uniform_int_distribution<int> dist(0, n - 1);

    int start = dist(rng);
    int end = dist(rng);
    if (start > end) swap(start, end);

    // Copiar segmento de parent1
    for (int i = start; i <= end; i++) {
        offspring[i] = parent1[i];
    }

    // Preencher com elementos de parent2
    vector<bool> used(n + 1, false);
    for (int i = start; i <= end; i++) {
        used[offspring[i]] = true;
    }

    int pos = (end + 1) % n;
    int parentPos = (end + 1) % n;

    while (pos != start) {
        while (used[parent2[parentPos]]) {
            parentPos = (parentPos + 1) % n;
        }
        offspring[pos] = parent2[parentPos];
        used[parent2[parentPos]] = true;
        pos = (pos + 1) % n;
        parentPos = (parentPos + 1) % n;
    }

    return offspring;
}

vector<int> PSO::twoPointCrossover(const vector<int>& parent1, const vector<int>& parent2) {
    int n = parent1.size();
    vector<int> offspring = parent1;

    uniform_int_distribution<int> dist(0, n - 1);
    int point1 = dist(rng);
    int point2 = dist(rng);
    if (point1 > point2) swap(point1, point2);

    for (int i = point1; i < point2; i++) {
        offspring[i] = parent2[i];
    }

    return offspring;
}

vector<int> PSO::pmxCrossover(const vector<int>& parent1, const vector<int>& parent2) {
    int n = parent1.size();
    vector<int> offspring(n);

    uniform_int_distribution<int> dist(0, n - 1);
    int point1 = dist(rng);
    int point2 = dist(rng);
    if (point1 > point2) swap(point1, point2);

    // Copiar segmento de parent1
    for (int i = point1; i <= point2; i++) {
        offspring[i] = parent1[i];
    }

    // Mapear elementos
    for (int i = point1; i <= point2; i++) {
        int val = parent2[i];
        bool found = false;
        for (int j = point1; j <= point2; j++) {
            if (offspring[j] == val) {
                found = true;
                break;
            }
        }

        if (!found) {
            int pos = i;
            while (pos >= point1 && pos <= point2) {
                // Encontrar onde val está em parent1
                int idx = -1;
                for (int j = 0; j < n; j++) {
                    if (parent1[j] == val) {
                        idx = j;
                        break;
                    }
                }
                val = parent2[idx];
                pos = idx;
            }
            offspring[pos] = parent2[i];
        }
    }

    // Preencher posições restantes
    vector<bool> used(n + 1, false);
    for (int i = 0; i < n; i++) {
        if (i >= point1 && i <= point2) {
            used[offspring[i]] = true;
        }
    }

    int idx2 = 0;
    for (int i = 0; i < n; i++) {
        if (i < point1 || i > point2) {
            while (used[parent2[idx2]]) idx2++;
            offspring[i] = parent2[idx2];
            used[parent2[idx2]] = true;
            idx2++;
        }
    }

    return offspring;
}

vector<int> PSO::ptlCrossover(const vector<int>& parent1, const vector<int>& parent2) {
    // PTL (Position-based Crossover com Three-parent Like)
    // Implementação simplificada: usar posições de parent1 com valores aleatórios de ambos
    int n = parent1.size();
    vector<int> offspring;
    vector<bool> used(n + 1, false);

    for (int i = 0; i < n; i++) {
        if (uniformDist(rng) < 0.5) {
            if (!used[parent1[i]]) {
                offspring.push_back(parent1[i]);
                used[parent1[i]] = true;
            }
        } else {
            if (!used[parent2[i]]) {
                offspring.push_back(parent2[i]);
                used[parent2[i]] = true;
            }
        }
    }

    // Preencher com elementos faltantes
    for (int i = 1; i <= n; i++) {
        if (!used[i]) {
            offspring.push_back(i);
        }
    }

    return offspring;
}

// ===== OPERADORES DE MUTAÇÃO =====

void PSO::swapMutation(vector<int>& solution) {
    if (uniformDist(rng) > mutationProb) return;

    uniform_int_distribution<int> dist(0, solution.size() - 1);
    int i = dist(rng);
    int j = dist(rng);

    swap(solution[i], solution[j]);
}

void PSO::insertMutation(vector<int>& solution) {
    if (uniformDist(rng) > mutationProb) return;

    uniform_int_distribution<int> dist(0, solution.size() - 1);
    int i = dist(rng);
    int j = dist(rng);

    if (i != j) {
        int element = solution[i];
        solution.erase(solution.begin() + i);
        solution.insert(solution.begin() + j, element);
    }
}

void PSO::multiSwapMutation(vector<int>& solution) {
    if (uniformDist(rng) > mutationProb) return;

    int numSwaps = uniformDist(rng) * 3 + 1;  // 1-3 swaps
    for (int s = 0; s < numSwaps; s++) {
        swapMutation(solution);
    }
}

void PSO::multiInsertMutation(vector<int>& solution) {
    if (uniformDist(rng) > mutationProb) return;

    int n = solution.size();
    uniform_int_distribution<int> dist(0, n - 1);

    int r1 = dist(rng);
    int r2 = dist(rng);

    if (r1 != r2) {
        int element = solution[r1];
        solution.erase(solution.begin() + r1);
        int insertPos = r2 > r1 ? r2 - 1 : r2;
        if (insertPos >= solution.size()) insertPos = solution.size() - 1;
        solution.insert(solution.begin() + insertPos, element);
    }
}

// ===== OPERADORES DE APRENDIZADO PSO =====

void PSO::learnFromHistoryMutation(vector<int>& position) {
    // Aplicar mutação (learning from history)
    switch (mutationOperator) {
        case 1: swapMutation(position); break;
        case 2: insertMutation(position); break;
        case 3: multiSwapMutation(position); break;
        case 4: multiInsertMutation(position); break;
        default: insertMutation(position);
    }
}

void PSO::learnFromLocalBestCrossover(const vector<int>& position, const vector<int>& localBest, vector<int>& newPosition) {
    // Aplicar crossover com local best
    switch (crossoverType) {
        case 1: newPosition = orderCrossover(position, localBest); break;
        case 2: newPosition = twoPointCrossover(position, localBest); break;
        case 3: newPosition = pmxCrossover(position, localBest); break;
        case 4: newPosition = ptlCrossover(position, localBest); break;
        default: newPosition = orderCrossover(position, localBest);
    }
}

void PSO::learnFromGlobalBestCrossover(const vector<int>& position, const vector<int>& globalBest, vector<int>& newPosition) {
    // Aplicar crossover com global best
    switch (crossoverType) {
        case 1: newPosition = orderCrossover(position, globalBest); break;
        case 2: newPosition = twoPointCrossover(position, globalBest); break;
        case 3: newPosition = pmxCrossover(position, globalBest); break;
        case 4: newPosition = ptlCrossover(position, globalBest); break;
        default: newPosition = orderCrossover(position, globalBest);
    }
}

// ===== ILS - LOCAL SEARCH =====

void PSO::ilsLocalSearch(vector<int>& solution) {
    // Fase de destruição + construção
    int n = solution.size();
    uniform_int_distribution<int> dist(0, n - 1);

    // Destruição: remover um elemento
    int r1 = dist(rng);
    int element = solution[r1];
    solution.erase(solution.begin() + r1);

    // Construção: inserir na melhor posição
    double bestFitness = numeric_limits<double>::max();
    int bestPos = 0;

    for (int pos = 0; pos <= (int)solution.size(); pos++) {
        vector<int> temp = solution;
        temp.insert(temp.begin() + pos, element);

        resetProblemData();
        double fit = evaluateParticle(temp);

        if (fit < bestFitness) {
            bestFitness = fit;
            bestPos = pos;
        }
    }

    solution.insert(solution.begin() + bestPos, element);
}

// ===== EXECUTAR ALGORITMO =====

void PSO::run(const string& instanceFile, const string& outputFile) {
    // Ler instância
    if (!readInstanceFromFile(instanceFile, problemData)) {
        cerr << "Erro ao ler instância" << endl;
        return;
    }

    // Inicializar global best
    globalBest.bestFitness = numeric_limits<double>::max();

    // Inicializar enxame
    cout << "Inicializando enxame..." << endl;
    initializeSwarm();

    auto startTime = chrono::high_resolution_clock::now();

    cout << "Executando PSO..." << endl;
    cout << fixed << setprecision(2);

    // Loop principal
    for (int gen = 0; gen < numGenerations; gen++) {
        double sumFitness = 0.0;
        double worstFitness = 0.0;

        // Atualizar cada partícula
        for (int p = 0; p < populationSize; p++) {
            // Step 5.1: Learning from history
            vector<int> newPos = swarm[p].position;
            learnFromHistoryMutation(newPos);

            // Step 5.2: Learning from local best
            if (uniformDist(rng) < c1) {
                learnFromLocalBestCrossover(newPos, swarm[p].bestPosition, newPos);
            }

            // Step 5.3: Learning from global best
            if (uniformDist(rng) < c2) {
                learnFromGlobalBestCrossover(newPos, globalBest.bestPosition, newPos);
            }

            // Aplicar ILS-based local search ao melhor da geração (opcional, melhora convergência)
            if (gen % 5 == 0) {  // A cada 5 gerações
                ilsLocalSearch(newPos);
            }

            // Avaliar nova posição
            resetProblemData();
            double newFitness = evaluateParticle(newPos);

            // Atualizar melhor pessoal
            if (newFitness < swarm[p].bestFitness) {
                swarm[p].bestFitness = newFitness;
                swarm[p].bestPosition = newPos;
            }

            // Atualizar posição
            swarm[p].position = newPos;
            swarm[p].fitness = newFitness;

            // Atualizar melhor global
            if (newFitness < globalBest.bestFitness) {
                globalBest.bestFitness = newFitness;
                globalBest.bestPosition = newPos;
            }

            sumFitness += swarm[p].fitness;
            worstFitness = max(worstFitness, swarm[p].fitness);
        }

        auto currentTime = chrono::high_resolution_clock::now();
        double elapsedTime = chrono::duration<double>(currentTime - startTime).count();

        double avgFitness = sumFitness / populationSize;

        // Armazenar estatísticas
        GenerationStats stats;
        stats.generation = gen;
        stats.bestFitness = globalBest.bestFitness;
        stats.avgFitness = avgFitness;
        stats.worstFitness = worstFitness;
        stats.elapsedTime = elapsedTime;
        generationHistory.push_back(stats);

        if (gen % 10 == 0 || gen == numGenerations - 1) {
            cout << "Gen " << gen << ": Best=" << globalBest.bestFitness
                 << " Avg=" << avgFitness << " Worst=" << worstFitness
                 << " Time=" << elapsedTime << "s" << endl;
        }
    }

    // Salvar resultados em CSV
    ofstream csvFile(outputFile);
    csvFile << "Generation,BestFitness,AvgFitness,WorstFitness,ElapsedTime" << endl;
    for (const auto& stats : generationHistory) {
        csvFile << stats.generation << ","
                << fixed << setprecision(1) << stats.bestFitness << ","
                << stats.avgFitness << ","
                << stats.worstFitness << ","
                << stats.elapsedTime << endl;
    }
    csvFile.close();

    cout << "\nResultados salvos em: " << outputFile << endl;
    cout << "Melhor solução encontrada: " << globalBest.bestFitness << endl;
}