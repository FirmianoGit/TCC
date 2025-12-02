# ============================================================================
# GERADOR MASSIVO DE INSTÂNCIAS PARA FLOWSHOP HÍBRIDO
# ============================================================================

import random
import os
from typing import List, Tuple, Dict

class HybridFlowshopInstanceGenerator:

    def __init__(self, seed: int = None):
        if seed is not None:
            random.seed(seed)

    def generate_instance(self,
                          n_jobs: int,
                          n_stages: int,
                          machines_per_stage: List[int],
                          processing_time_range: Tuple[int, int] = (1, 99),
                          machine_type: str = 'unrelated') -> Dict:

        if len(machines_per_stage) != n_stages:
            raise ValueError(f"machines_per_stage deve ter {n_stages} elementos")

        min_time, max_time = processing_time_range
        processing_times = []

        for stage in range(n_stages):
            stage_times = []
            n_machines = machines_per_stage[stage]

            for job in range(n_jobs):
                if machine_type == 'identical':
                    base_time = random.randint(min_time, max_time)
                    job_times = [base_time] * n_machines

                elif machine_type == 'uniform':
                    base_time = random.randint(min_time, max_time)
                    speeds = [random.uniform(0.5, 2.0) for _ in range(n_machines)]
                    job_times = [int(base_time / speed) for speed in speeds]

                elif machine_type == 'unrelated':
                    job_times = [random.randint(min_time, max_time)
                                 for _ in range(n_machines)]
                else:
                    raise ValueError(f"machine_type '{machine_type}' inválido")

                stage_times.append(job_times)

            processing_times.append(stage_times)

        return {
            'n_jobs': n_jobs,
            'n_stages': n_stages,
            'machines_per_stage': machines_per_stage,
            'processing_times': processing_times,
            'machine_type': machine_type
        }

    def save_to_txt(self, instance: Dict, filename: str):
        with open(filename, 'w') as f:
            f.write(f"{instance['n_jobs']} {instance['n_stages']}\n")
            f.write(' '.join(map(str, instance['machines_per_stage'])) + '\n')

            for stage_idx, stage_data in enumerate(instance['processing_times']):
                f.write(f"# Stage {stage_idx + 1}\n")
                for job_times in stage_data:
                    f.write(' '.join(map(str, job_times)) + '\n')

    def generate_initial_permutation(self, n_jobs: int, seed: int = None) -> List[int]:
        if seed is not None:
            random.seed(seed)

        permutation = list(range(n_jobs))
        random.shuffle(permutation)
        return permutation

    def save_permutation_to_txt(self, permutation: List[int], filename: str):
        with open(filename, 'w') as f:
            f.write(' '.join(map(str, permutation)) + '\n')


if __name__ == '__main__':
    print("=" * 80)
    print("GERADOR MASSIVO DE INSTÂNCIAS PARA TCC")
    print("=" * 80)

    # Diretórios de saída
    instances_dir = r"C:\Users\Firmiano\Desktop\TCC\ImplementacaoGA\Instancias"
    permutations_dir = r"C:\Users\Firmiano\Desktop\TCC\ImplementacaoGA\Permutacoes"

    # Criar diretórios se não existirem
    os.makedirs(instances_dir, exist_ok=True)
    os.makedirs(permutations_dir, exist_ok=True)

    # Configurações de benchmark (AUMENTADO!)
    benchmark_configs = [
        # (n_jobs, n_stages, machines_per_stage, count, name)
        (10, 3, [2, 2, 2], 30, 'P'),      # 30 instâncias pequenas
        (20, 3, [3, 3, 2], 30, 'M'),      # 30 instâncias médias
        (30, 4, [2, 3, 3, 2], 20, 'G'),   # 20 instâncias grandes
        (50, 5, [3, 4, 3, 4, 2], 20, 'XG'), # 20 instâncias extra grandes
    ]

    base_seed = 42
    total_count = 0

    # Arquivo de índice
    index_file = os.path.join(os.path.dirname(instances_dir), "benchmark_index.txt")

    with open(index_file, 'w') as idx:
        idx.write("# BENCHMARK INDEX - TCC FLOWSHOP HIBRIDO\n")
        idx.write("# Format: ID instance_file permutation_file n_jobs n_stages machines seed\n")
        idx.write(f"# Base path: {instances_dir}\n\n")

        for n_jobs, n_stages, machines, count, size_code in benchmark_configs:
            print(f"\nGerando {count} instâncias: {n_jobs} jobs, {n_stages} estágios ({size_code})")
            idx.write(f"# {size_code}: {n_jobs} jobs, {n_stages} estágios, {count} instâncias\n")

            for i in range(count):
                total_count += 1
                seed = base_seed + total_count

                # Nomenclatura simples: I1, I2, I3, ... / P1, P2, P3, ...
                instance_file = os.path.join(instances_dir, f"I{total_count}.txt")
                perm_file = os.path.join(permutations_dir, f"P{total_count}.txt")

                # Gerar instância
                generator = HybridFlowshopInstanceGenerator(seed=seed)
                instance = generator.generate_instance(
                    n_jobs=n_jobs,
                    n_stages=n_stages,
                    machines_per_stage=machines,
                    processing_time_range=(10, 50),
                    machine_type='unrelated'
                )

                generator.save_to_txt(instance, instance_file)

                # Gerar permutação
                perm = generator.generate_initial_permutation(n_jobs, seed=seed+10000)
                generator.save_permutation_to_txt(perm, perm_file)

                # Escrever no índice
                machines_str = ','.join(map(str, machines))
                idx.write(f"{total_count} I{total_count}.txt P{total_count}.txt "
                          f"{n_jobs} {n_stages} [{machines_str}] {seed}\n")

                if (i+1) % 10 == 0:
                    print(f"  {i+1}/{count} concluído...")

            idx.write("\n")

    print(f"\n{'=' * 80}")
    print(f"TOTAL: {total_count} instâncias geradas")
    print(f"{'=' * 80}")
    print(f"\nArquivos criados em:")
    print(f"  Instâncias:  {instances_dir}")
    print(f"  Permutações: {permutations_dir}")
    print(f"  Índice:      {index_file}")
    print("\nPronto para uso no AG!")
