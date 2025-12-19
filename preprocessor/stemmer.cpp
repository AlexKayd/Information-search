// g++ -std=c++17 -O2 stemmer.cpp -o stemmer.exe
// .\stemmer.exe

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <windows.h>
#include <chrono>
#include <iomanip>

void setup_utf8_console() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

// загрузка стоп слов
std::vector<std::string> load_stop_words() {
    std::vector<std::string> stops;
    std::ifstream file("stop_words.txt", std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Файл stop_words.txt не найден. Стоп-слова не будут удалены.\n";
        return stops;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) {
            stops.push_back(line);
        }
    }
    return stops;
}

// является ли стем стоп словом
bool is_stop_word(const std::string& term, const std::vector<std::string>& stop_words) {
    for (const auto& sw : stop_words) {
        if (term == sw) {
            return true;
        }
    }
    return false;
}

bool ends_with(const std::string& word, const std::string& suffix) {
    if (suffix.length() > word.length()) return false;
    return word.compare(word.length() - suffix.length(), suffix.length(), suffix) == 0;
}

void sort_by_length_desc(std::vector<std::string>& vec) {
    for (size_t i = 0; i < vec.size() - 1; ++i) {
        size_t max_idx = i;
        for (size_t j = i + 1; j < vec.size(); ++j) {
            if (vec[j].length() > vec[max_idx].length()) {
                max_idx = j;
            }
        }
        if (max_idx != i) {
            std::string temp = std::move(vec[i]);
            vec[i] = std::move(vec[max_idx]);
            vec[max_idx] = std::move(temp);
        }
    }
}

// подсчет UTF-8 символов
size_t utf8_char_count(const std::string& s) {
    size_t count = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if ((c & 0x80) == 0) {
            count++;
        } else if ((c & 0xE0) == 0xC0) {
            count++;
            i++;
        } else if ((c & 0xF0) == 0xE0) {
            count++;
            i += 2;
        } else if ((c & 0xF8) == 0xF0) {
            count++;
            i += 3;
        }
    }
    return count;
}

std::string stem(const std::string& word) {
    if (word.length() < 3) return word;
    
    std::string w = word;

    // слова исключения
    static const std::vector<std::string> exceptions = {
        "быть", "есть", "мочь", "хотеть", "знать", "идти", "дать", "видеть", "думать", "сказать"
    };
    for (const auto& e : exceptions) {
        if (w == e) return w;
    }
    
    struct SpecialRule {
        std::string suffix;
        std::string replacement;
    };
    
    static const std::vector<SpecialRule> special_rules = {
        {"ирование", "ир"},
        {"ование", "ир"},
        {"ание", ""},
        {"ение", ""},
        {"ться", ""},
        {"ться", ""},
        {"тель", ""},
        {"ник", ""},
        {"щик", ""},
        {"ция", "ц"},
        {"ки", "к"},
        {"ка", "к"},
        {"ала", ""},
        {"онный", "он"},
        {"нный", "н"},
        {"ия", ""},
        {"ие", ""},
        {"ающ", ""},
        {"ющ", ""}
    };

    // пробую специальные правила
    for (const auto& rule : special_rules) {
        if (ends_with(w, rule.suffix)) {
            std::string stemmed = w.substr(0, w.length() - rule.suffix.length()) + rule.replacement;
            if (stemmed.length() >= 2 && utf8_char_count(stemmed) >= 2) {
                return stemmed;
            }
        }
    }
    
    // удаление -ся, -сь
    if (w.length() >= 4) {
        if (ends_with(w, "ся") || ends_with(w, "сь")) {
            w = w.substr(0, w.length() - 4);
            if (w.length() < 4) return w;
            if (ends_with(w, "ть") && w.length() > 4) {
                return w.substr(0, w.length() - 4);
            }
        }
    }
    
    static std::vector<std::string> suffixes = {
        "ующийся", "ующаяся", "ующееся", "ющиеся",
        "овавшийся", "евавшийся", "ивавшийся",
        "оваться", "еваться", "иваться",
        "ующий", "ующая", "ующее", "ющих",
        "емый", "емая", "емое", "емыми",
        "имый", "имая", "имое", "имыми",
        "ивший", "ившая", "ившее", "ившие",
        "ывший", "ывшая", "ывшее", "ывшие",
        "вший", "вшая", "вшее", "вшие",
        "анный", "янный", "енный", "онный",
        "аешь", "аете", "ается", "аются", "ающий", "ающая", "ающее", "ающие",
        "ишь", "ите", "ится", "ятся", "ищий", "ищая", "ищее", "ищие",
        "ешь", "ете", "ется", "ются", "ющий", "ющая", "ущее", "ющие",
        "ить", "еть", "ать", "ять", "уть", "оть",
        "ит", "ет", "ат", "ят", "ут", "ют",
        "ла", "ло", "ли", "ал", "ял", "ил", "ел", "ол",
        "ем", "ом", "им", "ым", "ом", "ем",
        "ость", "ости", "остью", "остей",
        "ство", "ства", "ству", "ством",
        "альный", "ельный", "ильный", "ольный",
        "ий", "ая", "ое", "ые", "ой", "ый", "ью", "ью",
        "его", "ему", "ими", "ем", 
        "ого", "ому", "ыми", "ых", "их"
    };

    static bool sorted = false;
    if (!sorted) {
        sort_by_length_desc(suffixes);
        sorted = true;
    }
    
    for (const auto& suf : suffixes) {
        if (ends_with(w, suf)) {
            std::string stemmed = w.substr(0, w.length() - suf.length());
            if (utf8_char_count(stemmed) >= 3) {
                return stemmed;
            }
        }
    }
    
    return w;
}

// void test_stemmer() {
//     std::vector<std::pair<std::string, std::string>> tests = {
//         {"кошки", "кошк"},
//         {"мама", "мама"},
//         {"мамы", "мамы"},
//         {"собаки", "собак"},
//         {"операция", "операц"},
//         {"программирование", "программир"},
//         {"читать", "чит"},
//         {"читаешь", "чит"},
//         {"бегать", "бег"},
//         {"мыться", "мы"},
//         {"готовиться", "готови"},
//         {"хороший", "хорош"},
//         {"хорошие", "хорош"},
//         {"читающий", "чит"},
//         {"работала", "работ"},
//         {"операционный", "операцион"},
//         {"мочь", "мочь"},
//         {"быть", "быть"},
//         {"делать", "дел"},
//         {"говорить", "говор"},
//         {"красивый", "красив"},
//         {"красивая", "красив"},
//         {"формирование", "формир"},
//         {"активность", "активн"},
//         {"равенство", "равен"},
//         {"читатель", "чита"},
//         {"работник", "работ"}
//     };
    
//     int passed = 0;
//     int total = tests.size();
    
//     for (const auto& t : tests) {
//         std::string result = stem(t.first);
//         if (result == t.second) {
//             passed++;
//             std::cout << t.first << "  " << result << std::endl;
//         } else {
//             std::cout << t.first << "  " << result << " (ожидалось: " << t.second << ")" << std::endl;
//         }
//     }
    
//     std::cout << "\nТесты пройдены: " << passed << "/" << total << std::endl;
// }

std::vector<std::string> read_tokens(const std::string& path) {
    std::vector<std::string> tokens;
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return tokens;
    
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            if (line.back() == '\r') line.pop_back();
            if (!line.empty()) tokens.push_back(line);
        }
    }
    return tokens;
}

void write_stems(const std::string& path, const std::vector<std::string>& stems) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Ошибка записи: " << path << std::endl;
        return;
    }
    for (const auto& s : stems) {
        file << s << '\n';
    }
}


int main() {
    setup_utf8_console();
    // test_stemmer();
    
    auto stop_words = load_stop_words();
    const std::string in_dir = "tokens";
    const std::string out_dir = "stems";
    
    if (!std::filesystem::exists(in_dir)) {
        std::cerr << "Папка " << in_dir << " не найдена\n";
        return 1;
    }
    
    std::filesystem::create_directories(out_dir);
    
    int processed_files = 0;
    int total_tokens = 0;
    int total_filtered = 0;
    size_t total_input_bytes = 0;
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(in_dir)) {
            if (entry.path().extension() != ".tokens") continue;
            
            std::ifstream file(entry.path().string(), std::ios::binary | std::ios::ate);
            size_t file_size = file.tellg();
            file.close();
            
            auto tokens = read_tokens(entry.path().string());
            std::vector<std::string> stems;
            stems.reserve(tokens.size());
            
            for (const auto& tok : tokens) {

                if (is_stop_word(tok, stop_words)) {
                    total_filtered++;
                    continue;
                }

                std::string stemmed = stem(tok);

                // удаляю слишком короткие стемы
                if (utf8_char_count(stemmed) < 2) {
                    total_filtered++;
                    continue;
                }

                stems.push_back(stemmed);
            }
            
            std::string out_name = entry.path().stem().string() + ".stems";
            std::string out_path = out_dir + "/" + out_name;

            write_stems(out_path, stems);
            
            processed_files++;
            total_tokens += tokens.size();
            total_input_bytes += file_size;
            
            if (processed_files % 1000 == 0) {
                auto now = std::chrono::high_resolution_clock::now();
                double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - start).count();
                double mb = total_input_bytes / (1024.0 * 1024.0);
                double speed = (elapsed > 0) ? mb / elapsed : 0.0;

                std::cout << "Обработано " << processed_files << " файлов, стем: " << (total_tokens - total_filtered) << " (отфильтровано: " << total_filtered << "), скорость: " << std::fixed << std::setprecision(2) << speed << " МБ/сек\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
    double mb = total_input_bytes / (1024.0 * 1024.0);
    double avg_speed = (elapsed > 0) ? mb / elapsed : 0.0;
    
    std::cout << "\nФайлов обработано: " << processed_files << "\n";
    std::cout << "Всего стем: " << (total_tokens - total_filtered) << "\n";
    std::cout << "Отфильтровано стоп-слов: " << total_filtered << "\n";
    std::cout << "Время выполнения: " << std::fixed << std::setprecision(2) << elapsed << " сек\n";
    std::cout << "Средняя скорость: " << std::fixed << std::setprecision(2) << avg_speed << " МБ/сек\n";
    
    return 0;
}