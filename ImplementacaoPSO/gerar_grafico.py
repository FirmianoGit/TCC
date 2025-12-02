
# # ═══════════════════════════════════════════════════════════════════════════════════════
# # GRÁFICO DE GANTT + TARDINESS - HYBRID FLOWSHOP SCHEDULING
# # Ferramenta: Python 3 + matplotlib
# # ═══════════════════════════════════════════════════════════════════════════════════════

# import matplotlib.pyplot as plt
# import matplotlib.patches as mpatches
# import numpy as np

# # ─────────────────────────────────────────────────────────────────────────────────────
# # 1. DADOS DA INSTÂNCIA
# # ─────────────────────────────────────────────────────────────────────────────────────

# # Permutação (sequência de tarefas)
# permutation = [11, 15, 6, 2, 1, 3, 5, 13, 4, 7, 9, 12, 14, 8, 10]

# # Tempos de processamento - Estágio 1 (1 máquina)
# stage1_times = {
#     1: 12, 2: 28, 3: 19, 4: 39, 5: 33, 6: 16, 7: 39, 8: 48, 9: 41,
#     10: 48, 11: 11, 12: 42, 13: 37, 14: 46, 15: 33
# }

# # Tempos de processamento - Estágio 2 (2 máquinas)
# stage2_times = {
#     1: [49, 45], 2: [37, 35], 3: [21, 13], 4: [16, 17], 5: [34, 19],
#     6: [16, 20], 7: [42, 41], 8: [18, 34], 9: [48, 42], 10: [27, 46],
#     11: [21, 34], 12: [14, 13], 13: [47, 13], 14: [21, 13], 15: [20, 28]
# }

# # DUE DATES (todas as tarefas com mesmo valor)
# due_date = 129  # Valor que resulta em TT próximo de 1945

# # ─────────────────────────────────────────────────────────────────────────────────────
# # 2. SIMULAÇÃO - CALCULAR TEMPOS DE CONCLUSÃO E TARDINESS
# # ─────────────────────────────────────────────────────────────────────────────────────

# class GanttSimulation:
#     def __init__(self, permutation, stage1_times, stage2_times, due_date):
#         self.permutation = permutation
#         self.stage1_times = stage1_times
#         self.stage2_times = stage2_times
#         self.due_date = due_date
#         self.events = []

#     def run(self):
#         machine_available_time_s1 = 0.0
#         machine_available_time_s2 = [0.0, 0.0]

#         job_completion_time_s1 = {}
#         job_completion_time_s2 = {}
#         job_tardiness = {}

#         # ═══════════════════════════════════════════════════════════════════════════════
#         # ESTÁGIO 1: Processamento sequencial (1 máquina)
#         # ═══════════════════════════════════════════════════════════════════════════════

#         for job_id in self.permutation:
#             processing_time = self.stage1_times[job_id]
#             start_time_s1 = machine_available_time_s1
#             end_time_s1 = start_time_s1 + processing_time
#             machine_available_time_s1 = end_time_s1
#             job_completion_time_s1[job_id] = end_time_s1

#             self.events.append({
#                 'job': job_id,
#                 'stage': 1,
#                 'machine': 1,
#                 'start': start_time_s1,
#                 'end': end_time_s1,
#                 'duration': processing_time
#             })

#         # ═══════════════════════════════════════════════════════════════════════════════
#         # ESTÁGIO 2: Processamento paralelo (2 máquinas)
#         # ═══════════════════════════════════════════════════════════════════════════════

#         for job_id in self.permutation:
#             earliest_start_s2 = job_completion_time_s1[job_id]

#             if machine_available_time_s2[0] <= machine_available_time_s2[1]:
#                 machine_id = 1
#                 machine_idx = 0
#             else:
#                 machine_id = 2
#                 machine_idx = 1

#             start_time_s2 = max(earliest_start_s2, machine_available_time_s2[machine_idx])
#             processing_time = self.stage2_times[job_id][machine_idx]
#             end_time_s2 = start_time_s2 + processing_time

#             machine_available_time_s2[machine_idx] = end_time_s2
#             job_completion_time_s2[job_id] = end_time_s2

#             # Calcular tardiness
#             tardiness = max(0, end_time_s2 - self.due_date)
#             job_tardiness[job_id] = tardiness

#             self.events.append({
#                 'job': job_id,
#                 'stage': 2,
#                 'machine': machine_id,
#                 'start': start_time_s2,
#                 'end': end_time_s2,
#                 'duration': processing_time,
#                 'tardiness': tardiness
#             })

#         makespan = max(job_completion_time_s2.values())
#         total_tardiness = sum(job_tardiness.values())

#         return job_completion_time_s1, job_completion_time_s2, job_tardiness, makespan, total_tardiness

# # ─────────────────────────────────────────────────────────────────────────────────────
# # 3. EXECUTAR SIMULAÇÃO
# # ─────────────────────────────────────────────────────────────────────────────────────

# simulation = GanttSimulation(permutation, stage1_times, stage2_times, due_date)
# comp_s1, comp_s2, tardiness, makespan, total_tardiness = simulation.run()

# print("\n" + "=" * 80)
# print("RESULTADOS DA SIMULAÇÃO")
# print("=" * 80)
# print(f"\nDue Date (d): {due_date} unidades")
# print(f"Makespan (Cmax): {makespan} unidades")
# print(f"Tardiness Total (TT): {total_tardiness} unidades")

# print(f"\nTempos de conclusão - Estágio 2 e Tardiness:")
# for job_id in permutation:
#     t = tardiness[job_id]
#     status = "⚠ ATRASO" if t > 0 else "✓ NO PRAZO"
#     print(f"  Tarefa {job_id:2d}: C={comp_s2[job_id]:6.1f}, T={t:6.1f} {status}")

# # ─────────────────────────────────────────────────────────────────────────────────────
# # 4. CRIAR GRÁFICO DE GANTT COM CORES BASEADAS EM TARDINESS
# # ─────────────────────────────────────────────────────────────────────────────────────

# fig = plt.figure(figsize=(18, 12))
# gs = fig.add_gridspec(3, 1, height_ratios=[1, 1.2, 1], hspace=0.35)

# ax1 = fig.add_subplot(gs[0])
# ax2 = fig.add_subplot(gs[1])
# ax3 = fig.add_subplot(gs[2])

# # Cores para as tarefas
# colors = plt.cm.tab20(np.linspace(0, 1, 15))
# job_colors = {permutation[i]: colors[i] for i in range(len(permutation))}

# # ═════════════════════════════════════════════════════════════════════════════════════
# # GRÁFICO 1: ESTÁGIO 1
# # ═════════════════════════════════════════════════════════════════════════════════════

# ax1.set_title('Diagrama de Gantt - Estágio 1 (1 Máquina)', fontsize=13, fontweight='bold')
# ax1.set_xlabel('Tempo', fontsize=11)
# ax1.set_ylabel('Máquina', fontsize=11)
# ax1.set_ylim(0.5, 1.5)
# ax1.set_yticks([1])
# ax1.set_yticklabels(['M1'])
# ax1.grid(True, alpha=0.3, axis='x')

# for event in simulation.events:
#     if event['stage'] == 1:
#         ax1.barh(event['machine'], event['duration'], left=event['start'],
#                 height=0.6, color=job_colors[event['job']], edgecolor='black',
#                 linewidth=1.5, alpha=0.8)
#         mid_point = event['start'] + event['duration'] / 2
#         ax1.text(mid_point, event['machine'], f"T{event['job']}", ha='center',
#                 va='center', fontsize=9, fontweight='bold', color='white')

# ax1.set_xlim(0, makespan * 1.05)

# # ═════════════════════════════════════════════════════════════════════════════════════
# # GRÁFICO 2: ESTÁGIO 2
# # ═════════════════════════════════════════════════════════════════════════════════════

# ax2.set_title('Diagrama de Gantt - Estágio 2 (2 Máquinas Paralelas)', fontsize=13, fontweight='bold')
# ax2.set_xlabel('Tempo', fontsize=11)
# ax2.set_ylabel('Máquina', fontsize=11)
# ax2.set_ylim(0.5, 2.5)
# ax2.set_yticks([1, 2])
# ax2.set_yticklabels(['M1', 'M2'])
# ax2.grid(True, alpha=0.3, axis='x')

# for event in simulation.events:
#     if event['stage'] == 2:
#         ax2.barh(event['machine'], event['duration'], left=event['start'],
#                 height=0.6, color=job_colors[event['job']], edgecolor='black',
#                 linewidth=1.5, alpha=0.8)
#         mid_point = event['start'] + event['duration'] / 2
#         ax2.text(mid_point, event['machine'], f"T{event['job']}", ha='center',
#                 va='center', fontsize=9, fontweight='bold', color='white')

# ax2.set_xlim(0, makespan * 1.05)

# # ═════════════════════════════════════════════════════════════════════════════════════
# # GRÁFICO 3: TARDINESS POR TAREFA
# # ═════════════════════════════════════════════════════════════════════════════════════

# ax3.set_title(f'Tardiness por Tarefa (Due Date = {due_date})', fontsize=13, fontweight='bold')
# ax3.set_xlabel('Tarefa', fontsize=11)
# ax3.set_ylabel('Tardiness (unidades)', fontsize=11)
# ax3.grid(True, alpha=0.3, axis='y')

# # Cores: verde para sem atraso, vermelho para com atraso
# bar_colors = []
# tardiness_values = []
# task_labels = []

# for job_id in permutation:
#     t = tardiness[job_id]
#     tardiness_values.append(t)
#     task_labels.append(f'T{job_id}')
#     bar_colors.append('red' if t > 0 else 'green')

# bars = ax3.bar(range(len(permutation)), tardiness_values, color=bar_colors, alpha=0.7, edgecolor='black')

# # Adicionar valor nas barras
# for i, (bar, value) in enumerate(zip(bars, tardiness_values)):
#     if value > 0:
#         ax3.text(bar.get_x() + bar.get_width()/2, value + 10, f'{int(value)}',
#                 ha='center', va='bottom', fontsize=9, fontweight='bold')

# ax3.set_xticks(range(len(permutation)))
# ax3.set_xticklabels(task_labels, rotation=45, ha='right')
# ax3.set_xlim(-0.5, len(permutation) - 0.5)

# # Adicionar linha horizontal em y=0
# ax3.axhline(y=0, color='black', linestyle='-', linewidth=0.5)

# # ─────────────────────────────────────────────────────────────────────────────────────
# # 5. ADICIONAR LEGENDA E INFORMAÇÕES
# # ─────────────────────────────────────────────────────────────────────────────────────

# legend_patches = [
#     mpatches.Patch(color=job_colors[job], label=f'Tarefa {job}')
#     for job in sorted(permutation)
# ]

# fig.legend(handles=legend_patches, loc='upper center', bbox_to_anchor=(0.5, 0.02),
#           ncol=8, fontsize=10, frameon=True)

# info_text = f'''
# Permutação: {permutation}
# Makespan (Cmax): {makespan} unidades | Due Date: {due_date} unidades | Tardiness Total: {total_tardiness} unidades
# '''

# fig.text(0.5, 0.98, info_text, ha='center', fontsize=11,
#         bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5),
#         family='monospace', verticalalignment='top')

# plt.tight_layout(rect=[0, 0.08, 1, 0.96])

# # ─────────────────────────────────────────────────────────────────────────────────────
# # 6. SALVAR E EXIBIR
# # ─────────────────────────────────────────────────────────────────────────────────────

# plt.savefig('gantt_scheduling_with_tardiness.png', dpi=300, bbox_inches='tight')
# print(f"\n✓ Gráfico salvo como: gantt_scheduling_with_tardiness.png")

# plt.show()

# print("\n" + "=" * 80)
# print("GRÁFICO GERADO COM SUCESSO!")
# print("=" * 80)


# GRÁFICO DE GANTT - MÁQUINAS PARALELAS NÃO RELACIONADAS COM SETUP
# Problema: n=6 tarefas, m=2 máquinas, tempos de setup dependentes
# Ferramenta: Python 3 + matplotlib





"""
GRÁFICO DE GANTT - MÁQUINAS PARALELAS NÃO RELACIONADAS
Ilustração simples: Mesmas tarefas em 2 máquinas com tempos diferentes
(SEM setup, SEM sequenciamento complexo)
"""

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

# ─────────────────────────────────────────────────────────────────────────────────────
# DADOS DO PROBLEMA
# ─────────────────────────────────────────────────────────────────────────────────────

n_jobs = 6
n_machines = 2

# Tempos de processamento: p[machine][job]
# Cada tarefa leva tempos DIFERENTES em cada máquina
p = {
    1: {1: 5, 2: 8, 3: 4, 4: 7, 5: 6, 6: 3},      # Máquina 1
    2: {1: 6, 2: 5, 3: 9, 4: 4, 5: 8, 6: 7}       # Máquina 2
}

# ─────────────────────────────────────────────────────────────────────────────────────
# SIMULAÇÃO: Atribuir cada tarefa à máquina com menor tempo disponível
# ─────────────────────────────────────────────────────────────────────────────────────

machine_available = {1: 0.0, 2: 0.0}
job_machine = {}
events = []

for job in range(1, n_jobs + 1):
    # Calcular tempo de conclusão em cada máquina
    times = {}
    for machine in range(1, n_machines + 1):
        proc_time = p[machine][job]
        start_time = machine_available[machine]
        end_time = start_time + proc_time
        times[machine] = {'start': start_time, 'end': end_time, 'proc': proc_time}
    
    # Escolher máquina que libera mais cedo
    best_machine = min(times.keys(), key=lambda m: times[m]['end'])
    
    # Atualizar
    best_times = times[best_machine]
    machine_available[best_machine] = best_times['end']
    job_machine[job] = best_machine
    
    # Registrar evento
    events.append({
        'job': job,
        'machine': best_machine,
        'start': best_times['start'],
        'end': best_times['end'],
        'proc': best_times['proc']
    })

makespan = max(machine_available.values())

# ─────────────────────────────────────────────────────────────────────────────────────
# IMPRIMIR RESULTADOS
# ─────────────────────────────────────────────────────────────────────────────────────

print("=" * 70)
print("MÁQUINAS PARALELAS NÃO RELACIONADAS - TEMPOS DIFERENTES")
print("=" * 70)
print(f"\nTempos de Processamento:")
print(f"  Máquina 1: {p[1]}")
print(f"  Máquina 2: {p[2]}")
print(f"\nMakespan: {makespan} unidades")
print(f"\nAlocação de tarefas:")
for job in range(1, n_jobs + 1):
    machine = job_machine[job]
    proc = p[machine][job]
    print(f"  Tarefa {job}: Máquina {machine}, Tempo={proc}u, Conclusão={events[job-1]['end']}")

# ─────────────────────────────────────────────────────────────────────────────────────
# CRIAR GRÁFICO DE GANTT
# ─────────────────────────────────────────────────────────────────────────────────────

fig, ax = plt.subplots(figsize=(14, 8))

# Cores para cada tarefa
colors = plt.cm.tab10(np.linspace(0, 1, n_jobs))
job_colors = {i+1: colors[i] for i in range(n_jobs)}

# Desenhar barras
for event in events:
    machine = event['machine']
    job = event['job']
    
    # Barra de processamento
    ax.barh(
        machine,
        event['proc'],
        left=event['start'],
        height=0.6,
        color=job_colors[job],
        edgecolor='black',
        linewidth=2,
        alpha=0.85
    )
    
    # Label da tarefa no meio da barra
    mid_point = event['start'] + event['proc'] / 2
    ax.text(
        mid_point,
        machine,
        f"T{job}",
        ha='center',
        va='center',
        fontsize=11,
        fontweight='bold',
        color='white'
    )
    
    # Tempo de processamento embaixo da barra
    ax.text(
        mid_point,
        machine - 0.35,
        f"{event['proc']}u",
        ha='center',
        va='top',
        fontsize=9,
        color='black',
        style='italic'
    )

# ─────────────────────────────────────────────────────────────────────────────────────
# FORMATAÇÃO DO GRÁFICO
# ─────────────────────────────────────────────────────────────────────────────────────

ax.set_title(
    'Diagrama de Gantt - Máquinas Paralelas Não Relacionadas\n(Mesmas Tarefas, Tempos Diferentes)',
    fontsize=14,
    fontweight='bold',
    pad=20
)
ax.set_xlabel('Tempo (unidades)', fontsize=12, fontweight='bold')
ax.set_ylabel('Máquina', fontsize=12, fontweight='bold')

# Eixo Y
ax.set_ylim(0.5, n_machines + 0.5)
ax.set_yticks([1, 2])
ax.set_yticklabels(['Máquina 1', 'Máquina 2'], fontsize=11)

# Eixo X
ax.set_xlim(0, makespan * 1.1)
ax.grid(True, alpha=0.3, axis='x', linestyle='--')

# ─────────────────────────────────────────────────────────────────────────────────────
# LEGENDA
# ─────────────────────────────────────────────────────────────────────────────────────

legend_patches = [
    mpatches.Patch(color=job_colors[i+1], label=f'Tarefa {i+1}')
    for i in range(n_jobs)
]

ax.legend(
    handles=legend_patches,
    loc='upper right',
    fontsize=10,
    ncol=2,
    framealpha=0.95
)

# ─────────────────────────────────────────────────────────────────────────────────────
# TABELA DE INFORMAÇÕES
# ─────────────────────────────────────────────────────────────────────────────────────

info_text = f"""
N = {n_jobs} tarefas  |  M = {n_machines} máquinas paralelas não relacionadas
Características: Tempos de processamento p[i][j] variam conforme máquina e tarefa
Makespan (Cmax): {makespan} unidades
"""

ax.text(
    0.5,
    -0.18,
    info_text,
    transform=ax.transAxes,
    fontsize=11,
    verticalalignment='top',
    horizontalalignment='center',
    bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.7, pad=0.8),
    family='monospace'
)

plt.tight_layout()

# ─────────────────────────────────────────────────────────────────────────────────────
# SALVAR E EXIBIR
# ─────────────────────────────────────────────────────────────────────────────────────

plt.savefig('gantt_maquinas_paralelas_simples.png', dpi=300, bbox_inches='tight')
print(f"\n✓ Gráfico salvo como: gantt_maquinas_paralelas_simples.png")

plt.show()