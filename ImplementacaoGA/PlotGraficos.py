import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import glob
import os
from datetime import datetime

# Definir diretório de resultados (relativo ao script)
script_dir = os.path.dirname(os.path.abspath(__file__))
results_dir = os.path.join(script_dir, "Resultados")
graficos_dir = os.path.join(script_dir, "Graficos")

# Verificar se o diretório de resultados existe
if not os.path.exists(results_dir):
    print(f"ERRO: Diretório não encontrado: {results_dir}")
    exit(1)

# Criar pasta Graficos se não existir
if not os.path.exists(graficos_dir):
    os.makedirs(graficos_dir)
    print(f"Pasta 'Graficos' criada em: {graficos_dir}")
else:
    print(f"Usando pasta 'Graficos' existente em: {graficos_dir}")

# Encontrar todos os arquivos que começam com "summary"
files = sorted(glob.glob(os.path.join(results_dir, "summary*.csv")))

# Verificar se encontrou arquivos
if not files:
    print(f"ERRO: Nenhum arquivo 'summary*.csv' encontrado em: {results_dir}")
    print(f"Arquivos disponíveis: {os.listdir(results_dir)}")
    exit(1)

print(f"\nEncontrados {len(files)} arquivo(s) para processar:")
for f in files:
    print(f"  - {os.path.basename(f)}")

# Configurar estilo
sns.set_style("whitegrid")

# Processar cada arquivo
for idx, csv_file in enumerate(files, 1):
    print(f"\n{'='*70}")
    print(f"Processando arquivo {idx}/{len(files)}: {os.path.basename(csv_file)}")
    print(f"{'='*70}")

    # Carregar dados
    try:
        df = pd.read_csv(csv_file)
        print(f"✓ Dados carregados: {len(df)} linhas")
    except Exception as e:
        print(f"✗ ERRO ao carregar CSV: {e}")
        continue

    # Extrair timestamp do nome do arquivo para usar no nome do gráfico
    filename_without_ext = os.path.splitext(os.path.basename(csv_file))[0]

    # Criar figura com subplots
    plt.rcParams['figure.figsize'] = (14, 10)
    fig = plt.figure(figsize=(14, 10))

    # 1. Gráfico: Initial vs Final Fitness por Instância
    plt.subplot(2, 3, 1)
    plt.plot(df['Instance'], df['InitialFitness'], 'o-', linewidth=2, markersize=4, label='Initial', alpha=0.7)
    plt.plot(df['Instance'], df['FinalFitness'], 's-', linewidth=2, markersize=4, label='Final', alpha=0.7)
    plt.xlabel('Instance')
    plt.ylabel('Fitness (Total Tardiness)')
    plt.title('Fitness Inicial vs Final por Instância')
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.xticks(rotation=45, ha='right')

    # 2. Gráfico: Improvement (%) por Instância
    plt.subplot(2, 3, 2)
    plt.bar(df['Instance'], df['Improvement(%)'], alpha=0.7, color='green')
    plt.xlabel('Instance')
    plt.ylabel('Melhoria (%)')
    plt.title('Percentual de Melhoria por Instância')
    plt.grid(True, alpha=0.3, axis='y')
    plt.xticks(rotation=45, ha='right')

    # 3. Gráfico: Tempo de Execução
    plt.subplot(2, 3, 3)
    plt.bar(df['Instance'], df['ExecutionTime_ms'], alpha=0.7, color='orange')
    plt.xlabel('Instance')
    plt.ylabel('Tempo (ms)')
    plt.title('Tempo de Execução por Instância')
    plt.grid(True, alpha=0.3, axis='y')
    plt.xticks(rotation=45, ha='right')

    # 4. Boxplot: Final Fitness por número de Jobs
    plt.subplot(2, 3, 4)
    jobs_unique = sorted(df['Jobs'].unique())
    data_boxplot = [df[df['Jobs']==jobs]['FinalFitness'].tolist() for jobs in jobs_unique]
    plt.boxplot(data_boxplot, labels=jobs_unique)
    plt.xlabel('Número de Jobs')
    plt.ylabel('Final Fitness (Tardiness)')
    plt.title('Distribuição de Fitness Final por Tamanho')
    plt.grid(True, alpha=0.3, axis='y')

    # 5. Boxplot: Improvement por número de Jobs
    plt.subplot(2, 3, 5)
    data_improvement = [df[df['Jobs']==jobs]['Improvement(%)'].tolist() for jobs in jobs_unique]
    plt.boxplot(data_improvement, labels=jobs_unique)
    plt.xlabel('Número de Jobs')
    plt.ylabel('Melhoria (%)')
    plt.title('Distribuição de Melhoria por Tamanho')
    plt.grid(True, alpha=0.3, axis='y')

    # 6. Scatter: Fitness Final vs Tempo de Execução
    plt.subplot(2, 3, 6)
    scatter = plt.scatter(df['ExecutionTime_ms'], df['FinalFitness'],
                          c=df['Jobs'], cmap='viridis', alpha=0.6, s=50)
    plt.colorbar(scatter, label='Número de Jobs')
    plt.xlabel('Tempo de Execução (ms)')
    plt.ylabel('Final Fitness (Tardiness)')
    plt.title('Fitness vs Tempo de Execução')
    plt.grid(True, alpha=0.3)

    # Ajustar layout
    plt.tight_layout()

    # Salvar gráfico
    output_filename = f"graficos_{filename_without_ext}.png"
    output_path = os.path.join(graficos_dir, output_filename)
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"✓ Gráfico salvo: {output_filename}")

    # Fechar figura para liberar memória
    plt.close(fig)

    # Exibir estatísticas resumidas
    print(f"\n  Estatísticas:")
    print(f"    - Total de instâncias: {len(df)}")
    print(f"    - Melhoria média: {df['Improvement(%)'].mean():.2f}%")
    print(f"    - Melhoria máxima: {df['Improvement(%)'].max():.2f}%")
    print(f"    - Melhoria mínima: {df['Improvement(%)'].min():.2f}%")
    print(f"    - Tempo médio: {df['ExecutionTime_ms'].mean():.2f} ms")
    print(f"    - Tempo total: {df['ExecutionTime_ms'].sum():.2f} ms")
    print(f"    - Fitness final médio: {df['FinalFitness'].mean():.2f}")

print(f"\n{'='*70}")
print(f"✓ Processamento concluído!")
print(f"✓ Todos os gráficos foram salvos em: {graficos_dir}")
print(f"{'='*70}")
