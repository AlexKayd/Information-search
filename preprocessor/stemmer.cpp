// g++ -std=c++17 -O2 stemmer.cpp -o stemmer.exe
// .\stemmer.exe

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <windows.h>
#include <chrono>
#include <iomanip>

void setup_utf8_console() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

bool ends_with(const std::string& word, const std::string& suffix) {
    if (suffix.length() > word.length()) return false;
    return word.compare(word.length() - suffix.length(), suffix.length(), suffix) == 0;
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
        {"иться", ""},
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
            if (stemmed.length() >= 2) {
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
    
    // основные суффиксы
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
        "ого", "ому", "ыми", "ых", "их",
        "а", "у", "ы", "о", "е", "и", "ь", "я", "й", "ю"
    };
    
    static bool sorted = false;
    if (!sorted) {
        std::sort(suffixes.begin(), suffixes.end(),
            [](const std::string& a, const std::string& b) {
                return a.length() > b.length();
            });
        sorted = true;
    }
    
    for (const auto& suf : suffixes) {
        if (w.length() > suf.length() + 1) {
            if (ends_with(w, suf)) {
                std::string stemmed = w.substr(0, w.length() - suf.length());
                if (stemmed.length() >= 2) {
                    return stemmed;
                }
            }
        }
    }
    
    return w;
}

// void test_stemmer() {
//     std::vector<std::pair<std::string, std::string>> tests = {
//         {"кошки", "кошк"},
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
    
    const std::string in_dir = "tokens";
    const std::string out_dir = "stems";
    
    if (!std::filesystem::exists(in_dir)) {
        std::cerr << "Папка " << in_dir << " не найдена\n";
        return 1;
    }
    
    std::filesystem::create_directories(out_dir);
    
    int processed_files = 0;
    int total_tokens = 0;
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
                stems.push_back(stem(tok));
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
                
                std::cout << "Обработано " << processed_files << " файлов, " << "стем: " << total_tokens << ", " << "скорость: " << std::fixed << std::setprecision(2)  << speed << " МБ/сек" << std::endl;
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
    std::cout << "Всего стем: " << total_tokens << "\n";
    std::cout << "Время выполнения: " << std::fixed << std::setprecision(2) << elapsed << " сек\n";
    std::cout << "Средняя скорость: " << std::fixed << std::setprecision(2) << avg_speed << " МБ/сек\n";
    
    return 0;
}