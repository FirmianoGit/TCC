#include "genetic_algorithm.h"
#include <algorithm>
#include <numeric>

GeneticAlgorithm::GeneticAlgorithm(const GAParameters &p, const ProblemData &data)
    : params(p), problemData(data), currentGeneration(0), generationsWithoutImprovement(0) {
    random_device rd;
    rng.seed(rd());
    history.clear();
}

void GeneticAlgorithm::initializePopulation() {
    population.clear();
    population.resize(params.populationSize);

    vector<int> baseChromosome(problemData.numJobs);
    iota(baseChromosome.begin(), baseChromosome.end(), 0);

    for (int i = 0; i < params.populationSize; ++i) {
        vector<int> chromosome = baseChromosome;
        shuffle(chromosome.begin(), chromosome.end(), rng);
        population[i] = Individual(chromosome);
    }
}

void GeneticAlgorithm::initializePopulationWithSeed(const vector<int> &seedChromosome) {
    population.clear();
    population.resize(params.populationSize);

    // Primeiro indivíduo é a solução seed
    population[0] = Individual(seedChromosome);

    // 30% da população: pequenas mutações do seed
    int mutatedCount = params.populationSize * 3 / 10;
    for (int i = 1; i < mutatedCount; ++i) {
        Individual mutated(seedChromosome);

        // Aplicar múltiplas mutações
        int numMutations = 1 + (i % 5);
        for (int m = 0; m < numMutations; ++m) {
            performMutation(mutated);
        }

        population[i] = mutated;
    }

    // 70% da população: completamente aleatórios
    vector<int> baseChromosome(problemData.numJobs);
    iota(baseChromosome.begin(), baseChromosome.end(), 0);

    for (int i = mutatedCount; i < params.populationSize; ++i) {
        vector<int> chromosome = baseChromosome;
        shuffle(chromosome.begin(), chromosome.end(), rng);
        population[i] = Individual(chromosome);
    }
}

void GeneticAlgorithm::evaluateIndividual(Individual &ind) {
    // CRÍTICO: Criar uma CÓPIA COMPLETA dos dados do problema
    ProblemData dataCopy;
    dataCopy.numJobs = problemData.numJobs;
    dataCopy.numStages = problemData.numStages;
    dataCopy.machinesPerStage = problemData.machinesPerStage;

    // Copiar jobs
    dataCopy.jobs.clear();
    dataCopy.jobs.reserve(problemData.numJobs);
    for (const auto &job: problemData.jobs) {
        dataCopy.jobs.push_back(job);
    }

    // Inicializar máquinas
    dataCopy.machines.clear();
    for (int i = 1; i <= dataCopy.numStages; ++i) {
        for (int l = 1; l <= dataCopy.machinesPerStage[i - 1]; ++l) {
            dataCopy.machines.emplace(make_pair(i, l), Machine(i, l));
        }
    }

    // Converter para 1-based
    vector<int> chromosome1Based = ind.chromosome;
    for (int &job: chromosome1Based) {
        job += 1;
    }

    // Decodificar
    ind.fitness = decodeChromosome(chromosome1Based, dataCopy);
}

void GeneticAlgorithm::evaluatePopulation() {
    for (auto &ind: population) {
        evaluateIndividual(ind);
    }

    auto bestIter = min_element(population.begin(), population.end());
    if (bestIter->fitness < bestSolution.fitness) {
        bestSolution = *bestIter;
        generationsWithoutImprovement = 0;
    } else {
        generationsWithoutImprovement++;
    }
}

void GeneticAlgorithm::recordGenerationStats(double elapsedTime) {
    GenerationStats stats;
    stats.generation = currentGeneration;
    stats.elapsedTime = elapsedTime;

    stats.bestFitness = min_element(population.begin(), population.end())->fitness;
    stats.worstFitness = max_element(population.begin(), population.end())->fitness;

    double sum = 0.0;
    for (const auto &ind: population) {
        sum += ind.fitness;
    }
    stats.avgFitness = sum / population.size();

    history.push_back(stats);
}

vector<Individual> GeneticAlgorithm::tournamentSelection() {
    vector<Individual> matingPool;
    matingPool.reserve(params.populationSize);

    uniform_int_distribution<int> dist(0, params.populationSize - 1);

    for (int i = 0; i < params.populationSize; ++i) {
        int idx1 = dist(rng);
        int idx2 = dist(rng);

        if (population[idx1].fitness < population[idx2].fitness) {
            matingPool.push_back(population[idx1]);
        } else {
            matingPool.push_back(population[idx2]);
        }
    }

    return matingPool;
}

vector<Individual> GeneticAlgorithm::rouletteWheelSelection() {
    vector<Individual> matingPool;
    matingPool.reserve(params.populationSize);

    double maxFitness = max_element(population.begin(), population.end())->fitness;
    double minFitness = min_element(population.begin(), population.end())->fitness;

    // Se todos têm o mesmo fitness, retornar cópias aleatórias
    if (maxFitness == minFitness) {
        uniform_int_distribution<int> dist(0, params.populationSize - 1);
        for (int i = 0; i < params.populationSize; ++i) {
            matingPool.push_back(population[dist(rng)]);
        }
        return matingPool;
    }

    vector<double> invertedFitness(params.populationSize);
    double totalFitness = 0.0;

    for (int i = 0; i < params.populationSize; ++i) {
        invertedFitness[i] = maxFitness - population[i].fitness + 1.0;
        totalFitness += invertedFitness[i];
    }

    uniform_real_distribution<double> dist(0.0, totalFitness);

    for (int i = 0; i < params.populationSize; ++i) {
        double spin = dist(rng);
        double cumulative = 0.0;

        for (int j = 0; j < params.populationSize; ++j) {
            cumulative += invertedFitness[j];
            if (cumulative >= spin) {
                matingPool.push_back(population[j]);
                break;
            }
        }
    }

    return matingPool;
}

vector<Individual> GeneticAlgorithm::performSelection() {
    if (params.selectionType == SelectionType::TOURNAMENT) {
        return tournamentSelection();
    } else {
        return rouletteWheelSelection();
    }
}

pair<Individual, Individual> GeneticAlgorithm::orderBasedCrossover(const Individual &p1, const Individual &p2) {
    int n = p1.chromosome.size();
    vector<int> child1(n, -1), child2(n, -1);

    vector<bool> mask(n);
    uniform_int_distribution<int> dist(0, 1);
    for (int i = 0; i < n; ++i) {
        mask[i] = dist(rng) == 1;
    }

    for (int i = 0; i < n; ++i) {
        if (mask[i]) {
            child1[i] = p1.chromosome[i];
            child2[i] = p2.chromosome[i];
        }
    }

    int pos1 = 0, pos2 = 0;
    for (int i = 0; i < n; ++i) {
        if (child1[i] == -1) {
            while (find(child1.begin(), child1.end(), p2.chromosome[pos1]) != child1.end()) {
                pos1++;
            }
            child1[i] = p2.chromosome[pos1++];
        }

        if (child2[i] == -1) {
            while (find(child2.begin(), child2.end(), p1.chromosome[pos2]) != child2.end()) {
                pos2++;
            }
            child2[i] = p1.chromosome[pos2++];
        }
    }

    return {Individual(child1), Individual(child2)};
}

pair<Individual, Individual> GeneticAlgorithm::partialMappedCrossover(const Individual &p1, const Individual &p2) {
    int n = p1.chromosome.size();

    uniform_int_distribution<int> dist(0, n - 1);
    int cut1 = dist(rng);
    int cut2 = dist(rng);
    if (cut1 > cut2) swap(cut1, cut2);

    vector<int> child1 = p1.chromosome;
    vector<int> child2 = p2.chromosome;

    map<int, int> mapping1, mapping2;
    for (int i = cut1; i <= cut2; ++i) {
        mapping1[p2.chromosome[i]] = p1.chromosome[i];
        mapping2[p1.chromosome[i]] = p2.chromosome[i];
    }

    for (int i = cut1; i <= cut2; ++i) {
        child1[i] = p2.chromosome[i];
        child2[i] = p1.chromosome[i];
    }

    for (int i = 0; i < n; ++i) {
        if (i >= cut1 && i <= cut2) continue;

        while (find(child1.begin() + cut1, child1.begin() + cut2 + 1, child1[i]) != child1.begin() + cut2 + 1) {
            child1[i] = mapping1[child1[i]];
        }

        while (find(child2.begin() + cut1, child2.begin() + cut2 + 1, child2[i]) != child2.begin() + cut2 + 1) {
            child2[i] = mapping2[child2[i]];
        }
    }

    return {Individual(child1), Individual(child2)};
}

pair<Individual, Individual> GeneticAlgorithm::similarBlock2PointCrossover(const Individual &p1, const Individual &p2) {
    return twoPointOrderCrossover(p1, p2);
}

pair<Individual, Individual> GeneticAlgorithm::onePointOrderCrossover(const Individual &p1, const Individual &p2) {
    int n = p1.chromosome.size();
    uniform_int_distribution<int> dist(1, n - 1);
    int cutPoint = dist(rng);

    vector<int> child1, child2;
    set<int> used1, used2;

    for (int i = 0; i < cutPoint; ++i) {
        child1.push_back(p1.chromosome[i]);
        child2.push_back(p2.chromosome[i]);
        used1.insert(p1.chromosome[i]);
        used2.insert(p2.chromosome[i]);
    }

    for (int i = 0; i < n; ++i) {
        if (used1.find(p2.chromosome[i]) == used1.end()) {
            child1.push_back(p2.chromosome[i]);
        }
        if (used2.find(p1.chromosome[i]) == used2.end()) {
            child2.push_back(p1.chromosome[i]);
        }
    }

    return {Individual(child1), Individual(child2)};
}

pair<Individual, Individual> GeneticAlgorithm::twoPointOrderCrossover(const Individual &p1, const Individual &p2) {
    int n = p1.chromosome.size();
    uniform_int_distribution<int> dist(0, n - 1);
    int cut1 = dist(rng);
    int cut2 = dist(rng);
    if (cut1 > cut2) swap(cut1, cut2);

    vector<int> child1(n, -1), child2(n, -1);
    set<int> used1, used2;

    for (int i = cut1; i <= cut2; ++i) {
        child1[i] = p1.chromosome[i];
        child2[i] = p2.chromosome[i];
        used1.insert(p1.chromosome[i]);
        used2.insert(p2.chromosome[i]);
    }

    int fill1 = (cut2 + 1) % n;
    int fill2 = (cut2 + 1) % n;

    for (int i = 0; i < n; ++i) {
        int idx = (cut2 + 1 + i) % n;

        if (used1.find(p2.chromosome[idx]) == used1.end()) {
            while (child1[fill1] != -1) fill1 = (fill1 + 1) % n;
            child1[fill1] = p2.chromosome[idx];
            fill1 = (fill1 + 1) % n;
        }

        if (used2.find(p1.chromosome[idx]) == used2.end()) {
            while (child2[fill2] != -1) fill2 = (fill2 + 1) % n;
            child2[fill2] = p1.chromosome[idx];
            fill2 = (fill2 + 1) % n;
        }
    }

    return {Individual(child1), Individual(child2)};
}

pair<Individual, Individual> GeneticAlgorithm::performCrossover(const Individual &p1, const Individual &p2) {
    switch (params.crossoverType) {
        case CrossoverType::OBX: return orderBasedCrossover(p1, p2);
        case CrossoverType::PMX: return partialMappedCrossover(p1, p2);
        case CrossoverType::SB2OX: return similarBlock2PointCrossover(p1, p2);
        case CrossoverType::OPX: return onePointOrderCrossover(p1, p2);
        case CrossoverType::TPX: return twoPointOrderCrossover(p1, p2);
        default: return orderBasedCrossover(p1, p2);
    }
}

void GeneticAlgorithm::insertMutation(Individual &ind) {
    int n = ind.chromosome.size();
    if (n < 2) return;

    uniform_int_distribution<int> dist(0, n - 1);

    int pos1 = dist(rng);
    int pos2 = dist(rng);

    // Garantir que pos1 != pos2
    while (pos1 == pos2) {
        pos2 = dist(rng);
    }

    int job = ind.chromosome[pos1];
    ind.chromosome.erase(ind.chromosome.begin() + pos1);

    // Ajustar pos2 se necessário
    if (pos2 > pos1) pos2--;

    ind.chromosome.insert(ind.chromosome.begin() + pos2, job);
}

void GeneticAlgorithm::interchangeMutation(Individual &ind) {
    int n = ind.chromosome.size();
    if (n < 2) return;

    uniform_int_distribution<int> dist(0, n - 1);

    int pos1 = dist(rng);
    int pos2 = dist(rng);

    // Garantir que pos1 != pos2
    while (pos1 == pos2) {
        pos2 = dist(rng);
    }

    swap(ind.chromosome[pos1], ind.chromosome[pos2]);
}

void GeneticAlgorithm::swapMutation(Individual &ind) {
    int n = ind.chromosome.size();
    if (n < 2) return;

    uniform_int_distribution<int> dist(0, n - 2);

    int pos = dist(rng);
    swap(ind.chromosome[pos], ind.chromosome[pos + 1]);
}

void GeneticAlgorithm::performMutation(Individual &ind) {
    switch (params.mutationType) {
        case MutationType::INSERT: insertMutation(ind);
            break;
        case MutationType::INTERCHANGE: interchangeMutation(ind);
            break;
        case MutationType::SWAP: swapMutation(ind);
            break;
    }
}

void GeneticAlgorithm::localSearch(Individual &ind) {
    int n = ind.chromosome.size();
    int maxEval = params.localSearchIntensity * n;
    int evalCount = 0;

    Individual current = ind;
    evaluateIndividual(current);

    Individual bestLocal = current;

    while (evalCount < maxEval) {
        Individual neighbor = current;
        insertMutation(neighbor);
        evaluateIndividual(neighbor);

        if (neighbor.fitness < bestLocal.fitness) {
            bestLocal = neighbor;
            current = neighbor;
        }

        evalCount++;
    }

    ind = bestLocal;
}

void GeneticAlgorithm::halfGenesMutation(Individual &ind) {
    int n = ind.chromosome.size();
    int halfN = max(1, n / 2);

    vector<int> indices(n);
    iota(indices.begin(), indices.end(), 0);
    shuffle(indices.begin(), indices.end(), rng);
    indices.resize(halfN);

    vector<int> selectedGenes;
    for (int idx: indices) {
        selectedGenes.push_back(ind.chromosome[idx]);
    }

    shuffle(selectedGenes.begin(), selectedGenes.end(), rng);

    for (int i = 0; i < halfN; ++i) {
        ind.chromosome[indices[i]] = selectedGenes[i];
    }
}

void GeneticAlgorithm::restartProcedure() {
    cout << "  -> Restart: Regenerando populacao com diversidade..." << endl;

    // Ordenar população
    sort(population.begin(), population.end());

    // Manter apenas os TOP 10% (mais elite)
    int eliteCount = max(1, params.populationSize / 10);

    vector<int> baseChromosome(problemData.numJobs);
    iota(baseChromosome.begin(), baseChromosome.end(), 0);

    uniform_int_distribution<int> eliteDist(0, eliteCount - 1);

    // ============ DIAGNÓSTICO: Verificar elite ============
    cout << "  -> Elite preservada (top " << eliteCount << "):" << endl;
    for (int i = 0; i < min(3, eliteCount); ++i) {
        cout << "     [" << i << "] Fitness=" << population[i].fitness << " Chr=[";
        for (int j = 0; j < min(8, (int) population[i].chromosome.size()); ++j) {
            cout << population[i].chromosome[j];
            if (j < min(8, (int) population[i].chromosome.size()) - 1) cout << ",";
        }
        cout << "...]" << endl;
    }
    // ======================================================

    // 10%-30%: Elite com 1-3 mutações
    for (int i = eliteCount; i < params.populationSize * 3 / 10; ++i) {
        int eliteIdx = eliteDist(rng);
        population[i] = population[eliteIdx];

        int numMutations = 1 + (rng() % 3);
        for (int m = 0; m < numMutations; ++m) {
            performMutation(population[i]);
        }
    }

    // 30%-50%: Elite com half genes mutation
    for (int i = params.populationSize * 3 / 10; i < params.populationSize / 2; ++i) {
        int eliteIdx = eliteDist(rng);
        population[i] = population[eliteIdx];
        halfGenesMutation(population[i]);
    }

    // 50%-100%: Completamente aleatórios (DIVERSIDADE FORTE)
    int randomStart = params.populationSize / 2;
    for (int i = randomStart; i < params.populationSize; ++i) {
        vector<int> chromosome = baseChromosome;
        shuffle(chromosome.begin(), chromosome.end(), rng);
        population[i] = Individual(chromosome);
    }

    // ============ DIAGNÓSTICO: Verificar aleatorios gerados ============
    cout << "  -> Primeiros 3 aleatorios gerados (indices " << randomStart << " a " << (randomStart + 2) << "):" <<
            endl;
    for (int i = randomStart; i < min(randomStart + 3, (int) population.size()); ++i) {
        cout << "     [" << i << "] Chr=[";
        for (int j = 0; j < min(8, (int) population[i].chromosome.size()); ++j) {
            cout << population[i].chromosome[j];
            if (j < min(8, (int) population[i].chromosome.size()) - 1) cout << ",";
        }
        cout << "...]" << endl;
    }
    // ===================================================================

    // Reavaliar TODA a população
    cout << "  -> Avaliando populacao regenerada..." << endl;
    evaluatePopulation();

    // ============ DIAGNÓSTICO: Verificar fitness após avaliação ============
    set<double> uniqueFit;
    for (const auto &ind: population) {
        uniqueFit.insert(ind.fitness);
    }

    auto minFit = min_element(population.begin(), population.end());
    auto maxFit = max_element(population.begin(), population.end());

    double sumFit = 0.0;
    for (const auto &ind: population) {
        sumFit += ind.fitness;
    }
    double avgFit = sumFit / population.size();

    cout << "  -> Populacao apos restart:" << endl;
    cout << "     Best=" << minFit->fitness << " Avg=" << avgFit << " Worst=" << maxFit->fitness << endl;
    cout << "     Fitness unicos: " << uniqueFit.size() << " / " << params.populationSize << endl;

    // Mostrar alguns fitness
    cout << "     Primeiros 5 fitness: [";
    for (int i = 0; i < min(5, (int) population.size()); ++i) {
        cout << population[i].fitness;
        if (i < 4) cout << ", ";
    }
    cout << "]" << endl;
    // =======================================================================

    generationsWithoutImprovement = 0;

    cout << "  -> Restart completo!" << endl;
}


bool GeneticAlgorithm::isDuplicate(const Individual &ind) {
    // MODIFICADO: Verificar apenas se já existe EXATAMENTE o mesmo cromossomo
    // Não bloquear indivíduos com mesmo fitness mas cromossomos diferentes
    int duplicateCount = 0;
    for (const auto &existing: population) {
        if (existing.chromosome == ind.chromosome) {
            duplicateCount++;
            if (duplicateCount >= 2) {
                // Permitir até 2 cópias
                return true;
            }
        }
    }
    return false;
}

int GeneticAlgorithm::getWorstIndex() {
    int worstIdx = 0;
    double worstFitness = population[0].fitness;

    for (int i = 1; i < params.populationSize; ++i) {
        if (population[i].fitness > worstFitness) {
            worstFitness = population[i].fitness;
            worstIdx = i;
        }
    }

    return worstIdx;
}

void GeneticAlgorithm::replaceWorst(const Individual &child) {
    int worstIdx = getWorstIndex();

    // Aceitar se for melhor OU igual (permite convergência temporária mas mantém pressão evolutiva)
    if (child.fitness <= population[worstIdx].fitness) {
        population[worstIdx] = child;
    }
}

// Individual GeneticAlgorithm::runWithSeed(const vector<int> &seedChromosome) {
//     auto startTime = chrono::high_resolution_clock::now();
//
//     cout << "\n========================================" << endl;
//     cout << "INICIANDO ALGORITMO GENETICO (COM SEED)" << endl;
//     cout << "========================================" << endl;
//     cout << "Selecao: " << selectionTypeToString(params.selectionType) << endl;
//     cout << "Crossover: " << crossoverTypeToString(params.crossoverType) << endl;
//     cout << "Mutacao: " << mutationTypeToString(params.mutationType) << endl;
//     cout << "Populacao: " << params.populationSize << endl;
//     cout << "Prob. Crossover: " << params.crossoverProb << endl;
//     cout << "Prob. Mutacao: " << params.mutationProb << endl;
//     cout << "========================================\n" << endl;
//
//     initializePopulationWithSeed(seedChromosome);
//     evaluatePopulation();
//
//     auto elapsed0 = chrono::high_resolution_clock::now() - startTime;
//     recordGenerationStats(chrono::duration<double>(elapsed0).count());
//
//     cout << "Populacao inicial avaliada:" << endl;
//     cout << "  Best: " << history[0].bestFitness << endl;
//     cout << "  Avg:  " << history[0].avgFitness << endl;
//     cout << "  Worst:" << history[0].worstFitness << endl;
//
//     // ============ DIAGNÓSTICO: Verificar diversidade inicial ============
//     set<double> uniqueFitness;
//     for (const auto &ind: population) {
//         uniqueFitness.insert(ind.fitness);
//     }
//     cout << "  Fitness unicos: " << uniqueFitness.size() << " / " << params.populationSize << endl;
//
//     // Mostrar primeiros 5 cromossomos
//     cout << "\nPrimeiros 5 cromossomos:" << endl;
//     for (int i = 0; i < min(5, (int) population.size()); ++i) {
//         cout << "  [" << i << "] Fitness=" << population[i].fitness << " Chr=[";
//         for (int j = 0; j < min(10, (int) population[i].chromosome.size()); ++j) {
//             cout << population[i].chromosome[j];
//             if (j < min(10, (int) population[i].chromosome.size()) - 1) cout << ",";
//         }
//         cout << "...]" << endl;
//     }
//     cout << endl;
//     // ====================================================================
//
//     uniform_real_distribution<double> randDist(0.0, 1.0);
//     currentGeneration = 0;
//
//     while (true) {
//         auto currentTime = chrono::high_resolution_clock::now();
//         chrono::duration<double> elapsed = currentTime - startTime;
//
//         if (elapsed.count() >= params.maxCPUTimeSeconds) {
//             cout << "\nTempo maximo atingido: " << elapsed.count() << "s" << endl;
//             break;
//         }
//
//         currentGeneration++;
//
//         vector<Individual> matingPool = performSelection();
//
//         // ============ DIAGNÓSTICO: Contar operações efetivas ============
//         int crossoverCount = 0;
//         int mutationCount = 0;
//         int replacementCount = 0;
//         // ================================================================
//
//         for (int i = 0; i < params.populationSize - 1; i += 2) {
//             Individual parent1 = matingPool[i];
//             Individual parent2 = matingPool[i + 1];
//
//             Individual child1, child2;
//
//             double randCross = randDist(rng);
//             if (randCross < params.crossoverProb) {
//                 auto children = performCrossover(parent1, parent2);
//                 child1 = children.first;
//                 child2 = children.second;
//                 crossoverCount++;
//             } else {
//                 child1 = parent1;
//                 child2 = parent2;
//             }
//
//             double randMut1 = randDist(rng);
//             if (randMut1 < params.mutationProb) {
//                 performMutation(child1);
//                 mutationCount++;
//             }
//
//             double randMut2 = randDist(rng);
//             if (randMut2 < params.mutationProb) {
//                 performMutation(child2);
//                 mutationCount++;
//             }
//
//             evaluateIndividual(child1);
//             evaluateIndividual(child2);
//
//             double beforeWorst = population[getWorstIndex()].fitness;
//             replaceWorst(child1);
//             if (population[getWorstIndex()].fitness < beforeWorst) replacementCount++;
//
//             beforeWorst = population[getWorstIndex()].fitness;
//             replaceWorst(child2);
//             if (population[getWorstIndex()].fitness < beforeWorst) replacementCount++;
//         }
//
//         auto bestIter = min_element(population.begin(), population.end());
//         if (bestIter->fitness < bestSolution.fitness) {
//             bestSolution = *bestIter;
//             generationsWithoutImprovement = 0;
//         } else {
//             generationsWithoutImprovement++;
//         }
//
//         recordGenerationStats(elapsed.count());
//
//         if (params.localSearchFreq != INT_MAX && currentGeneration % params.localSearchFreq == 0) {
//             auto bestIter = min_element(population.begin(), population.end());
//             int bestIdx = distance(population.begin(), bestIter);
//
//             double beforeLS = population[bestIdx].fitness;
//             localSearch(population[bestIdx]);
//             double afterLS = population[bestIdx].fitness;
//
//             if (population[bestIdx].fitness < bestSolution.fitness) {
//                 bestSolution = population[bestIdx];
//                 generationsWithoutImprovement = 0;
//                 cout << "Geracao " << currentGeneration << ": Busca local melhorou! "
//                         << beforeLS << " -> " << afterLS << endl;
//             }
//         }
//
//         if (params.restartGenerations != INT_MAX && generationsWithoutImprovement >= params.restartGenerations) {
//             cout << "Geracao " << currentGeneration << ": Restart acionado (sem melhoria por "
//                     << generationsWithoutImprovement << " geracoes)" << endl;
//             restartProcedure();
//         }
//
//         if (currentGeneration % 100 == 0) {
//             // ============ DIAGNÓSTICO DETALHADO ============
//             set<double> uniqueFit;
//             for (const auto &ind: population) {
//                 uniqueFit.insert(ind.fitness);
//             }
//
//             cout << "Geracao " << currentGeneration << ":" << endl;
//             cout << "  Best=" << bestSolution.fitness
//                     << " | Avg=" << history.back().avgFitness
//                     << " | Worst=" << history.back().worstFitness << endl;
//             cout << "  Fitness unicos: " << uniqueFit.size() << " / " << params.populationSize << endl;
//             cout << "  Operacoes: Crossover=" << crossoverCount
//                     << " Mutacao=" << mutationCount
//                     << " Substituicoes=" << replacementCount << endl;
//             cout << "  Tempo: " << elapsed.count() << "s" << endl;
//             // ================================================
//         }
//     }
//
//     cout << "\n========================================" << endl;
//     cout << "ALGORITMO GENETICO FINALIZADO" << endl;
//     cout << "========================================" << endl;
//     cout << "Total de geracoes: " << currentGeneration << endl;
//     cout << "Melhor fitness: " << bestSolution.fitness << endl;
//     cout << "Diversidade final (Worst-Best): " << (history.back().worstFitness - history.back().bestFitness) << endl;
//     cout << "========================================\n" << endl;
//
//     return bestSolution;
// }

Individual GeneticAlgorithm::runWithSeed(const vector<int> &seedChromosome) {
    auto startTime = chrono::high_resolution_clock::now();

    cout << "\n========================================" << endl;
    cout << "INICIANDO ALGORITMO GENETICO (COM SEED)" << endl;
    cout << "========================================" << endl;
    cout << "Selecao: " << selectionTypeToString(params.selectionType) << endl;
    cout << "Crossover: " << crossoverTypeToString(params.crossoverType) << endl;
    cout << "Mutacao: " << mutationTypeToString(params.mutationType) << endl;
    cout << "Populacao: " << params.populationSize << endl;
    cout << "Prob. Crossover: " << params.crossoverProb << endl;
    cout << "Prob. Mutacao: " << params.mutationProb << endl;
    cout << "========================================\n" << endl;

    initializePopulationWithSeed(seedChromosome);
    evaluatePopulation();

    auto elapsed0 = chrono::high_resolution_clock::now() - startTime;
    recordGenerationStats(chrono::duration<double>(elapsed0).count());

    cout << "Populacao inicial avaliada:" << endl;
    cout << "  Best: " << history[0].bestFitness << endl;
    cout << "  Avg:  " << history[0].avgFitness << endl;
    cout << "  Worst:" << history[0].worstFitness << endl;

    set<double> uniqueFitness;
    for (const auto &ind: population) {
        uniqueFitness.insert(ind.fitness);
    }
    cout << "  Fitness unicos: " << uniqueFitness.size() << " / " << params.populationSize << endl;

    cout << "\nPrimeiros 5 cromossomos:" << endl;
    for (int i = 0; i < min(5, (int) population.size()); ++i) {
        cout << "  [" << i << "] Fitness=" << population[i].fitness << " Chr=[";
        for (int j = 0; j < min(10, (int) population[i].chromosome.size()); ++j) {
            cout << population[i].chromosome[j];
            if (j < min(10, (int) population[i].chromosome.size()) - 1) cout << ",";
        }
        cout << "...]" << endl;
    }
    cout << endl;

    uniform_real_distribution<double> randDist(0.0, 1.0);
    currentGeneration = 0;

    while (true) {
        auto currentTime = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = currentTime - startTime;

        if (elapsed.count() >= params.maxCPUTimeSeconds) {
            cout << "\nTempo maximo atingido: " << elapsed.count() << "s" << endl;
            break;
        }

        currentGeneration++;

        vector<Individual> matingPool = performSelection();

        // ============ CALCULAR DIVERSIDADE E ADAPTAR PARÂMETROS ============
        auto minFitIter = min_element(population.begin(), population.end());
        auto maxFitIter = max_element(population.begin(), population.end());
        double diversity = maxFitIter->fitness - minFitIter->fitness;

        // Temperatura para simulated annealing (diminui com o tempo)
        double temperature = max(1.0, 50.0 * (1.0 - elapsed.count() / params.maxCPUTimeSeconds));

        // Adaptar taxa de mutação baseado na diversidade
        double adaptiveMutationProb = params.mutationProb;
        if (diversity < 50.0) adaptiveMutationProb = min(0.15, params.mutationProb * 3.0);
        if (diversity < 10.0) adaptiveMutationProb = min(0.25, params.mutationProb * 5.0);
        if (diversity < 1.0) adaptiveMutationProb = min(0.4, params.mutationProb * 10.0);

        int crossoverCount = 0;
        int mutationCount = 0;
        int replacementCount = 0;
        int forcedReplacementCount = 0;
        // ===================================================================


        for (int i = 0; i < params.populationSize - 1; i += 2) {
            Individual parent1 = matingPool[i];
            Individual parent2 = matingPool[i + 1];

            Individual child1, child2;

            if (randDist(rng) < params.crossoverProb) {
                auto children = performCrossover(parent1, parent2);
                child1 = children.first;
                child2 = children.second;
                crossoverCount++;
            } else {
                child1 = parent1;
                child2 = parent2;
            }

            if (randDist(rng) < adaptiveMutationProb) {
                performMutation(child1);
                mutationCount++;
            }

            if (randDist(rng) < adaptiveMutationProb) {
                performMutation(child2);
                mutationCount++;
            }

            evaluateIndividual(child1);
            evaluateIndividual(child2);

            // ============ NOVA ESTRATÉGIA DE SUBSTITUIÇÃO ============
            // Encontrar índice do pior
            int worstIdx = getWorstIndex();
            double worstFitness = population[worstIdx].fitness;

            // Child1
            if (child1.fitness <= worstFitness) {
                // Aceitar se igual ou melhor
                population[worstIdx] = child1;
                replacementCount++;
            } else {
                // Aceitar pior com probabilidade baseada em temperatura (Simulated Annealing)
                double delta = child1.fitness - worstFitness;
                double acceptanceProb = exp(-delta / temperature);

                if (randDist(rng) < acceptanceProb) {
                    population[worstIdx] = child1;
                    forcedReplacementCount++;
                }
            }

            // Recalcular pior para child2
            worstIdx = getWorstIndex();
            worstFitness = population[worstIdx].fitness;

            // Child2
            if (child2.fitness <= worstFitness) {
                population[worstIdx] = child2;
                replacementCount++;
            } else {
                double delta = child2.fitness - worstFitness;
                double acceptanceProb = exp(-delta / temperature);

                if (randDist(rng) < acceptanceProb) {
                    population[worstIdx] = child2;
                    forcedReplacementCount++;
                }
            }
            // =========================================================
        }

        auto bestIter = min_element(population.begin(), population.end());
        if (bestIter->fitness < bestSolution.fitness) {
            bestSolution = *bestIter;
            generationsWithoutImprovement = 0;
        } else {
            generationsWithoutImprovement++;
        }

        recordGenerationStats(elapsed.count());

        if (params.localSearchFreq != INT_MAX && currentGeneration % params.localSearchFreq == 0) {
            auto bestIter = min_element(population.begin(), population.end());
            int bestIdx = distance(population.begin(), bestIter);

            double beforeLS = population[bestIdx].fitness;
            localSearch(population[bestIdx]);
            double afterLS = population[bestIdx].fitness;

            if (population[bestIdx].fitness < bestSolution.fitness) {
                bestSolution = population[bestIdx];
                generationsWithoutImprovement = 0;
                cout << "Geracao " << currentGeneration << ": Busca local melhorou! "
                        << beforeLS << " -> " << afterLS << endl;
            }
        }

        if (params.restartGenerations != INT_MAX && generationsWithoutImprovement >= params.restartGenerations) {
            cout << "Geracao " << currentGeneration << ": Restart acionado (sem melhoria por "
                    << generationsWithoutImprovement << " geracoes)" << endl;
            restartProcedure();
        }

        if (currentGeneration % 100 == 0) {
            set<double> uniqueFit;
            for (const auto &ind: population) {
                uniqueFit.insert(ind.fitness);
            }

            cout << "Geracao " << currentGeneration << ":" << endl;
            cout << "  Best=" << bestSolution.fitness
                    << " | Avg=" << history.back().avgFitness
                    << " | Worst=" << history.back().worstFitness << endl;
            cout << "  Fitness unicos: " << uniqueFit.size() << " / " << params.populationSize << endl;
            cout << "  Diversidade: " << diversity << " | Temperatura: " << fixed << setprecision(2) << temperature <<
                    endl;
            cout << "  Operacoes: Cross=" << crossoverCount
                    << " Mut=" << mutationCount
                    << " | Subst=" << replacementCount
                    << " Forcada=" << forcedReplacementCount << endl;
            cout << "  MutProb adaptativa: " << fixed << setprecision(3) << adaptiveMutationProb << endl;
            cout << "  Tempo: " << elapsed.count() << "s" << endl;
        }
    }

    cout << "\n========================================" << endl;
    cout << "ALGORITMO GENETICO FINALIZADO" << endl;
    cout << "========================================" << endl;
    cout << "Total de geracoes: " << currentGeneration << endl;
    cout << "Melhor fitness: " << bestSolution.fitness << endl;
    cout << "Diversidade final (Worst-Best): " << (history.back().worstFitness - history.back().bestFitness) << endl;
    cout << "========================================\n" << endl;

    return bestSolution;
}


Individual GeneticAlgorithm::run() {
    vector<int> emptyChromosome;
    return runWithSeed(emptyChromosome);
}

string selectionTypeToString(SelectionType type) {
    switch (type) {
        case SelectionType::TOURNAMENT: return "Tournament";
        case SelectionType::ROULETTE_WHEEL: return "Roulette";
        default: return "Unknown";
    }
}

string crossoverTypeToString(CrossoverType type) {
    switch (type) {
        case CrossoverType::OBX: return "OBX";
        case CrossoverType::PMX: return "PMX";
        case CrossoverType::SB2OX: return "SB2OX";
        case CrossoverType::OPX: return "OPX";
        case CrossoverType::TPX: return "TPX";
        default: return "Unknown";
    }
}

string mutationTypeToString(MutationType type) {
    switch (type) {
        case MutationType::INSERT: return "Insert";
        case MutationType::INTERCHANGE: return "Interchange";
        case MutationType::SWAP: return "Swap";
        default: return "Unknown";
    }
}
