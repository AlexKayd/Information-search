// g++ -std=c++17 -O2 tokenizer.cpp -o tokenizer.exe
// .\tokenizer.exe
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <filesystem>
#include <chrono>
#include <vector>
#include <iomanip>
#include <windows.h>
#include <cctype>

void setup_utf8_console() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

std::unordered_set<std::string> load_known_abbrevs() {
    std::unordered_set<std::string> result;
    std::ifstream file("known_abbrevs.txt", std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "known_abbrevs.txt не найден.\n";
        return result;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        line.erase(0, line.find_first_not_of(" \t\n\r"));
        line.erase(line.find_last_not_of(" \t\n\r") + 1);
        
        if (!line.empty()) {
            result.insert(line);
        }
    }
    return result;
}

bool is_valid_char(unsigned char c, unsigned char next = 0) {
    // Английские буквы
    if (c < 128) {
        return std::isalnum(c) || c == '-';
    }
    
    // Кириллица
    if ((c == 0xD0 || c == 0xD1) && next >= 0x80 && next <= 0xBF) {
        return true;
    }
    
    return false;
}

// приведение к нижнему регистру
std::string to_lower_utf8(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c1 = static_cast<unsigned char>(s[i]);
        
        // Английские буквы
        if (c1 >= 'A' && c1 <= 'Z') {
            result.push_back(c1 + 32);
            i++;
        }
        // Кириллица
        else if (c1 == 0xD0 && i + 1 < s.size()) {
            unsigned char c2 = static_cast<unsigned char>(s[i + 1]);
            
            if (c2 >= 0x90 && c2 <= 0x9F) {
                result.push_back(0xD0);
                result.push_back(c2 + 0x20);
                i += 2;
            }
            else if (c2 >= 0xA0 && c2 <= 0xAF) {
                result.push_back(0xD1);
                result.push_back(c2 - 0x20);
                i += 2;
            }
            else if (c2 == 0x81) {
                result.push_back(0xD1);
                result.push_back(0x91);
                i += 2;
            }
            else {
                result.push_back(c1);
                result.push_back(c2);
                i += 2;
            }
        }
        else if (c1 == 0xD1 && i + 1 < s.size()) {
            unsigned char c2 = static_cast<unsigned char>(s[i + 1]);
            result.push_back(c1);
            result.push_back(c2);
            i += 2;
        }
        else {
            result.push_back(c1);
            i++;
        }
    }
    
    return result;
}

bool is_valid_token(const std::string& token) {
    if (token.empty()) return false;
    
    bool has_letter_or_digit = false;
    for (size_t i = 0; i < token.size(); ) {
        unsigned char c = static_cast<unsigned char>(token[i]);
        
        // цифры
        if (c >= '0' && c <= '9') {
            has_letter_or_digit = true;
            i++;
        }
        // английские буквы
        else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            has_letter_or_digit = true;
            i++;
        }
        // кириллица
        else if (c == 0xD0 || c == 0xD1) {
            if (i + 1 < token.size()) {
                unsigned char c2 = static_cast<unsigned char>(token[i + 1]);

                if ((c == 0xD0 && c2 >= 0x90 && c2 <= 0xBF) || (c == 0xD1 && c2 >= 0x80 && c2 <= 0x8F) || (c == 0xD1 && c2 == 0x91)) {
                    has_letter_or_digit = true;
                }
            }
            i += 2;
        }
        // дефис внутри токена
        else if (c == '-' && i > 0 && i < token.size() - 1) {
            i++;
        }
        else {
            i++;
        }
    }
    
    return has_letter_or_digit && token.length() >= 1;
}

std::vector<std::string> tokenize(const std::string& text, const std::unordered_set<std::string>& known_abbrevs) {
    if (text.empty()) return {};

    const size_t MAX_TEXT_LENGTH = 100000;
    std::string processed = (text.length() > MAX_TEXT_LENGTH) ? text.substr(0, MAX_TEXT_LENGTH) : text;

    std::vector<std::string> tokens;
    std::string current;

    for (size_t i = 0; i < processed.size(); ) {
        unsigned char c = static_cast<unsigned char>(processed[i]);
        
        unsigned char next = (i + 1 < processed.size()) ? static_cast<unsigned char>(processed[i + 1]) : 0;

        if (is_valid_char(c, next)) {
            current.push_back(c);
            
            if ((c == 0xD0 || c == 0xD1) && next >= 0x80) {
                current.push_back(next);
                i += 2;
            } else {
                i++;
            }
        } else {
            if (!current.empty()) {
                while (!current.empty() && current.front() == '-') {
                    current.erase(0, 1);
                }
                while (!current.empty() && current.back() == '-') {
                    current.pop_back();
                }
                
                if (!current.empty()) {
                    // является ли токен аббревиатурой
                    bool is_abbreviation = false;
                    if (known_abbrevs.count(current)) {
                        tokens.push_back(current);
                        is_abbreviation = true;
                    } 
                    else {
                        std::string lower_current = to_lower_utf8(current);
                        for (const auto& abbrev : known_abbrevs) {
                            if (to_lower_utf8(abbrev) == lower_current) {
                                tokens.push_back(abbrev);
                                is_abbreviation = true;
                                break;
                            }
                        }
                    }
                    
                    if (!is_abbreviation) {
                        std::string lower = to_lower_utf8(current);
                        
                        if (is_valid_token(lower)) {
                            tokens.push_back(lower);
                        }
                    }
                }
                current.clear();
            }
            
            if (c >= '0' && c <= '9') {
                current.push_back(c);
            }
            
            i++;
        }
    }

    if (!current.empty()) {
        while (!current.empty() && current.front() == '-') {
            current.erase(0, 1);
        }
        while (!current.empty() && current.back() == '-') {
            current.pop_back();
        }
        
        if (!current.empty()) {
            bool is_abbreviation = false;
            if (known_abbrevs.count(current)) {
                tokens.push_back(current);
                is_abbreviation = true;
            } else {
                std::string lower_current = to_lower_utf8(current);
                for (const auto& abbrev : known_abbrevs) {
                    if (to_lower_utf8(abbrev) == lower_current) {
                        tokens.push_back(abbrev);
                        is_abbreviation = true;
                        break;
                    }
                }
            }
            
            if (!is_abbreviation) {
                std::string lower = to_lower_utf8(current);
                if (is_valid_token(lower)) {
                    tokens.push_back(lower);
                }
            }
        }
    }

    return tokens;
}

void save_tokens(int doc_id, const std::vector<std::string>& tokens) {
    std::filesystem::create_directories("tokens");
    std::string path = "tokens/" + std::to_string(doc_id) + ".tokens";
    std::ofstream out(path, std::ios::binary);
    for (const auto& t : tokens) {
        out << t << '\n';
    }
}

std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

size_t count_utf8_chars(const std::string& str) {
    size_t chars = 0;
    for (size_t i = 0; i < str.size(); ++chars) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if (c < 0x80) i += 1;
        else if ((c & 0xE0) == 0xC0) i += 2;
        else if ((c & 0xF0) == 0xE0) i += 3;
        else if ((c & 0xF8) == 0xF0) i += 4;
        else i += 1;
    }
    return chars;
}


int main() {
    setup_utf8_console();
    
    auto known_abbrevs = load_known_abbrevs();
    int processed_docs = 0;
    long long total_tokens = 0;
    long long total_token_chars = 0;
    long long total_input_bytes = 0;

    auto start = std::chrono::high_resolution_clock::now();

    try {
        for (const auto& entry : std::filesystem::directory_iterator("docs")) {
            if (entry.path().extension() != ".txt") continue;

            std::string text = read_file(entry.path().string());
            if (text.empty()) continue;

            total_input_bytes += text.size();
            auto tokens = tokenize(text, known_abbrevs);
            if (tokens.empty()) continue;

            std::string stem = entry.path().stem().string();
            int doc_id = std::stoi(stem);
            save_tokens(doc_id, tokens);

            total_tokens += tokens.size();
            for (const auto& t : tokens) {
                total_token_chars += count_utf8_chars(t);
            }
            processed_docs++;

            if (processed_docs % 1000 == 0) {
                auto now = std::chrono::high_resolution_clock::now();
                double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - start).count();
                double kb = total_input_bytes / 1024.0;
                double speed = (elapsed > 0) ? kb / elapsed : 0.0;
                std::cout << "Обработано " << processed_docs << " документов, токенов: " << total_tokens << ", скорость: " << std::fixed << std::setprecision(2) << speed << " КБ/сек\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }

    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
    double total_kb = total_input_bytes / 1024.0;
    double speed_kb_sec = (duration > 0) ? total_kb / duration : 0.0;
    double avg_len = total_tokens > 0 ? static_cast<double>(total_token_chars) / total_tokens : 0.0;

    std::cout << "\nДокументов обработано: " << processed_docs << "\n";
    std::cout << "Всего токенов: " << total_tokens << "\n";
    std::cout << "Средняя длина токена: " << std::fixed << std::setprecision(2) << avg_len << " символов\n";
    std::cout << "Время выполнения: " << std::fixed << std::setprecision(2) << duration << " сек\n";
    std::cout << "Скорость токенизации: " << std::fixed << std::setprecision(2) << speed_kb_sec << " КБ/сек\n";

    return 0;
}