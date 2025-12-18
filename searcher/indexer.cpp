// g++ -std=c++17 -O2 indexer.cpp -o indexer.exe
// .\indexer.exe

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <windows.h>
#include <chrono>

void setup_utf8_console() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

// сортировка вставками
void insertion_sort_terms(std::vector<std::string>& v) {
    for (size_t i = 1; i < v.size(); ++i) {
        std::string key = std::move(v[i]);
        size_t j = i;
        while (j > 0 && v[j - 1] > key) {
            v[j] = std::move(v[j - 1]);
            --j;
        }
        v[j] = std::move(key);
    }
}

// удаление дубликатов
std::vector<std::string> remove_term_duplicates(const std::vector<std::string>& tokens) {
    if (tokens.empty()) return {};
    std::vector<std::string> sorted = tokens;
    insertion_sort_terms(sorted);

    std::vector<std::string> unique;
    unique.push_back(std::move(sorted[0]));
    for (size_t i = 1; i < sorted.size(); ++i) {
        if (sorted[i] != sorted[i - 1]) {
            unique.push_back(std::move(sorted[i]));
        }
    }
    return unique;
}

// хэш-таблица
struct SimpleHashTable {
    struct Entry {
        std::string key;
        size_t value;
        bool occupied = false;
    };

    std::vector<Entry> table;
    size_t size = 0;

    SimpleHashTable(size_t capacity = 1048576) {
        table.resize(capacity);
    }

    size_t hash(const std::string& s) const {
        size_t h = 0;
        for (char c : s) {
            h = h * 31 + static_cast<unsigned char>(c);
        }
        return h % table.size();
    }

    size_t* find(const std::string& key) {
        size_t idx = hash(key);
        size_t start = idx;
        do {
            if (!table[idx].occupied) {
                return nullptr;
            }
            if (table[idx].key == key) {
                return &table[idx].value;
            }
            idx = (idx + 1) % table.size();
        } while (idx != start);
        return nullptr;
    }

    bool insert(const std::string& key, size_t value) {
        size_t idx = hash(key);
        size_t start = idx;
        do {
            if (!table[idx].occupied) {
                table[idx].key = key;
                table[idx].value = value;
                table[idx].occupied = true;
                size++;
                return true;
            }
            if (table[idx].key == key) {
                return false;
            }
            idx = (idx + 1) % table.size();
        } while (idx != start);
        return false;
    }
};

// сортировка лексикографически
void sort_terms_lex(std::vector<std::string>& terms, std::vector<std::vector<int>>& postings) {
    size_t n = terms.size();
    for (size_t i = 0; i < n - 1; ++i) {
        size_t min_idx = i;
        for (size_t j = i + 1; j < n; ++j) {
            if (terms[j] < terms[min_idx]) {
                min_idx = j;
            }
        }
        if (min_idx != i) {
            std::string temp_term = std::move(terms[i]);
            terms[i] = std::move(terms[min_idx]);
            terms[min_idx] = std::move(temp_term);

            std::vector<int> temp_post = std::move(postings[i]);
            postings[i] = std::move(postings[min_idx]);
            postings[min_idx] = std::move(temp_post);
        }
    }
}

std::vector<std::string> read_stems(const std::string& path) {
    std::vector<std::string> stems;
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return stems;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) stems.push_back(line);
    }
    return stems;
}

void write_index(const std::string& path, const std::vector<std::string>& terms, const std::vector<std::vector<int>>& postings) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return;
    for (size_t i = 0; i < terms.size(); ++i) {
        out << terms[i] << ":";
        for (size_t j = 0; j < postings[i].size(); ++j) {
            if (j > 0) out << ",";
            out << postings[i][j];
        }
        out << "\n";
    }
}

void validate_index(const std::vector<std::string>& terms, const std::vector<std::vector<int>>& postings, size_t sample_count = 10) {
    if (terms.empty()) {
        std::cout << "Индекс пуст.\n";
        return;
    }
    std::cout << "\nПримеры из индекса\n";
    size_t n = terms.size();
    size_t start_idx = (n > sample_count) ? (n / 2 - sample_count / 2) : 0;
    size_t end_idx = std::min(start_idx + sample_count, n);
    for (size_t i = start_idx; i < end_idx; ++i) {
        std::cout << terms[i] << ": ";
        for (size_t j = 0; j < postings[i].size(); ++j) {
            if (j > 0) std::cout << ",";
            std::cout << postings[i][j];
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

int main() {
    setup_utf8_console();

    const std::string input_dir = "../preprocessor/stems";
    const std::string output_file = "boolean_index.txt";

    if (!std::filesystem::exists(input_dir)) {
        std::cerr << "Папка stems не найдена\n";
        return 1;
    }

    std::vector<std::string> all_terms;
    std::vector<std::vector<int>> all_postings;
    SimpleHashTable term_to_index(1048576);

    auto start = std::chrono::high_resolution_clock::now();

    try {
        size_t processed_docs = 0;

        for (const auto& entry : std::filesystem::directory_iterator(input_dir)) {
            if (entry.path().extension() != ".stems") continue;

            std::string stem_name = entry.path().stem().string();
            int doc_id = std::stoi(stem_name);

            auto stems = read_stems(entry.path().string());
            if (stems.empty()) continue;

            stems = remove_term_duplicates(stems);

            for (const auto& term : stems) {
                size_t* idx_ptr = term_to_index.find(term);
                if (idx_ptr) {
                    all_postings[*idx_ptr].push_back(doc_id);
                } else {
                    size_t new_idx = all_terms.size();
                    all_terms.push_back(term);
                    all_postings.push_back({doc_id});
                    term_to_index.insert(term, new_idx);
                }
            }

            processed_docs++;
            if (processed_docs % 1000 == 0) {
                std::cout << "Документов обработано: " << processed_docs << "\n";
            }
        }

        std::cout << "Сортировка терминов\n";
        sort_terms_lex(all_terms, all_postings);

        std::cout << "Сортировка posting листов\n";
        for (size_t i = 0; i < all_postings.size(); ++i) {
            auto& list = all_postings[i];
            if (list.size() <= 1) continue;

            for (size_t idx = 1; idx < list.size(); ++idx) {
                int key = list[idx];
                size_t j = idx;
                while (j > 0 && list[j - 1] > key) {
                    list[j] = list[j - 1];
                    --j;
                }
                list[j] = key;
            }

            std::vector<int> unique_list;
            unique_list.push_back(list[0]);
            for (size_t k = 1; k < list.size(); ++k) {
                if (list[k] != list[k - 1]) {
                    unique_list.push_back(list[k]);
                }
            }
            list = std::move(unique_list);
        }

        std::cout << "Сохранение индекса\n";
        write_index(output_file, all_terms, all_postings);

        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();

        std::cout << "\nИндексация завершена.\n";
        std::cout << "Всего терминов: " << all_terms.size() << "\n";
        std::cout << "Документов обработано: " << processed_docs << "\n";
        std::cout << "Время выполнения: " << elapsed << " сек\n";
        validate_index(all_terms, all_postings, 10);

    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }

    return 0;
}