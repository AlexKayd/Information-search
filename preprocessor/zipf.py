# python zipf.py
import os
from collections import Counter
import matplotlib.pyplot as plt

TOKENS_DIR = "tokens"

all_tokens = []
for filename in os.listdir(TOKENS_DIR):
    if filename.endswith(".tokens"):
        with open(os.path.join(TOKENS_DIR, filename), "r", encoding="utf-8") as f:
            tokens = [line.strip() for line in f if line.strip()]
            all_tokens.extend(tokens)

print(f"Всего токенов: {len(all_tokens)}")
print(f"Уникальных токенов: {len(set(all_tokens))}")

freq_counter = Counter(all_tokens)
freq_list = freq_counter.most_common()
frequencies = [freq for token, freq in freq_list]
ranks = list(range(1, len(frequencies) + 1))

plt.rcParams['font.family'] = 'Arial'

plt.figure(figsize=(10, 6))
plt.loglog(ranks, frequencies, 'b.', markersize=1, alpha=0.6)
plt.title("Закон Ципфа: распределение частотности терминов")
plt.xlabel("Ранг (r)")
plt.ylabel("Частота (f)")
plt.grid(True, which="both", ls="--", linewidth=0.5)
plt.tight_layout()
plt.show()