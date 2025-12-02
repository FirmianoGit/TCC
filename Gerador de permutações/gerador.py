# ============================================================================
# GERADOR DE INSTÂNCIAS PARA FLOWSHOP HÍBRIDO - COMPATÍVEL COM C++
# ============================================================================
# Arquivo: gerador_hfs_para_cpp.py
# Descrição: Gera instâncias de HFS e permutações iniciais para AG em C++
# ============================================================================

import random
from typing import List, Tuple, Dict

class HybridFlowshopInstanceGenerator:
    '''
    Gerador de instâncias para Flowshop Híbrido
    Compatível com leitura em C++
    '''
    
    def __init__(self, seed: int = None):
        if seed is not None:
            random.seed(seed)
    
    def generate_instance(self, 
                         n_jobs: int,
                         n_stages: int,
                         machines_per_stage: List[int],
                         processing_time_range: Tuple[int, int] = (1, 99),
                         machine_type: str = 'unrelated') -> Dict:
        '''
        Gera uma instância de flowshop híbrido
        
        Parâmetros:
        -----------
        n_jobs: int - Número de tarefas
        n_stages: int - Número de estágios
        machines_per_stage: List[int] - Máquinas em cada estágio [M1, M2, ..., Mg]
        processing_time_range: Tuple[int, int] - Intervalo [min, max] dos tempos
        machine_type: str - 'identical', 'uniform', ou 'unrelated'
        
        Retorna: dict com a instância
        '''
        
        if len(machines_per_stage) != n_stages:
            raise ValueError(f"machines_per_stage deve ter {n_stages} elementos")
        
        min_time, max_time = processing_time_range
        processing_times = []
        
        # Gerar tempos de processamento: processing_times[stage][job][machine]
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
        '''
        Salva instância em formato texto para leitura em C++
        
        Formato do arquivo:
        -------------------
        Linha 1: n_jobs n_stages
        Linha 2: M1 M2 M3 ... Mg (número de máquinas por estágio)
        
        Depois, para cada estágio:
          Para cada job:
            p[job][maq1] p[job][maq2] ... p[job][maqM]
        '''
        
        with open(filename, 'w') as f:
            # Cabeçalho
            f.write(f"{instance['n_jobs']} {instance['n_stages']}\n")
            
            # Número de máquinas por estágio
            f.write(' '.join(map(str, instance['machines_per_stage'])) + '\n')
            
            # Tempos de processamento por estágio
            for stage_idx, stage_data in enumerate(instance['processing_times']):
                f.write(f"# Stage {stage_idx + 1}\n")
                for job_times in stage_data:
                    f.write(' '.join(map(str, job_times)) + '\n')
    
    def generate_initial_permutation(self, n_jobs: int, seed: int = None) -> List[int]:
        '''
        Gera uma permutação inicial de tarefas (para usar como semente no AG)
        
        Retorna: lista [0, 1, 2, ..., n-1] embaralhada
        '''
        if seed is not None:
            random.seed(seed)
        
        permutation = list(range(n_jobs))
        random.shuffle(permutation)
        return permutation
    
    def save_permutation_to_txt(self, permutation: List[int], filename: str):
        '''
        Salva permutação em arquivo texto (uma linha)
        '''
        with open(filename, 'w') as f:
            f.write(' '.join(map(str, permutation)) + '\n')


# ============================================================================
# EXEMPLO DE USO: GERAR BENCHMARK COMPLETO
# ============================================================================

if __name__ == '__main__':
    print("=" * 80)
    print("GERADOR DE INSTÂNCIAS PARA ALGORITMO GENÉTICO EM C++")
    print("=" * 80)
    
    # Configuração do benchmark
    benchmark_configs = [
        # (n_jobs, n_stages, machines_per_stage, nome)
        (10, 3, [2, 2, 2], 'small'),
        (20, 3, [3, 3, 2], 'medium'),
        (30, 4, [2, 3, 3, 2], 'large'),
        (50, 5, [3, 4, 3, 4, 2], 'xlarge'),
    ]
    
    instances_per_config = 5  # 5 instâncias de cada tamanho
    base_seed = 42
    
    # Gerar todas as instâncias
    instance_count = 0
    
    for n_jobs, n_stages, machines, size_name in benchmark_configs:
        print(f"\nGerando instâncias {size_name}: {n_jobs} jobs, {n_stages} estágios")
        
        for i in range(instances_per_config):
            instance_count += 1
            seed = base_seed + instance_count
            
            # Nome dos arquivos
            instance_file = f"instance_{size_name}_{i+1:02d}.txt"
            perm_file = f"permutation_{size_name}_{i+1:02d}.txt"
            
            # Criar gerador com seed
            generator = HybridFlowshopInstanceGenerator(seed=seed)
            
            # Gerar instância
            instance = generator.generate_instance(
                n_jobs=n_jobs,
                n_stages=n_stages,
                machines_per_stage=machines,
                processing_time_range=(10, 50),
                machine_type='unrelated'
            )
            
            # Salvar instância
            generator.save_to_txt(instance, instance_file)
            
            # Gerar e salvar permutação inicial
            perm = generator.generate_initial_permutation(n_jobs, seed=seed+1000)
            generator.save_permutation_to_txt(perm, perm_file)
            
            print(f"  ✓ {instance_file} + {perm_file} (seed={seed})")
    
    print(f"\n{'=' * 80}")
    print(f"TOTAL: {instance_count} instâncias geradas")
    print(f"{'=' * 80}")
    
    # Criar arquivo de índice
    print("\nCriando arquivo de índice (benchmark_index.txt)...")
    with open('benchmark_index.txt', 'w') as f:
        f.write("# BENCHMARK INDEX\n")
        f.write("# Format: instance_file permutation_file n_jobs n_stages seed\n")
        f.write(f"# Total instances: {instance_count}\n")
        f.write("\n")
        
        instance_count = 0
        for n_jobs, n_stages, machines, size_name in benchmark_configs:
            f.write(f"# {size_name.upper()} INSTANCES\n")
            for i in range(instances_per_config):
                instance_count += 1
                seed = base_seed + instance_count
                instance_file = f"instance_{size_name}_{i+1:02d}.txt"
                perm_file = f"permutation_{size_name}_{i+1:02d}.txt"
                f.write(f"{instance_file} {perm_file} {n_jobs} {n_stages} {seed}\n")
            f.write("\n")
    
    print("✓ benchmark_index.txt criado")
    print("\nPronto! Execute os arquivos no seu programa C++")
