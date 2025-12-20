// g++ -std=c++17 -O2 searcher.cpp `
//    -I"C:\Program Files\PostgreSQL\16\include" `
//    -L"C:\Program Files\PostgreSQL\16\lib" `
//    -lpq `
//    -o searcher.exe

// .\searcher.exe
// .\searcher.exe --ids-only 

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cctype>
#include <windows.h>
#include <libpq-fe.h>

void setup_utf8_console() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

// поиск

std::vector<int> intersect_lists(const std::vector<int>& a, const std::vector<int>& b) {
    std::vector<int> result;
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) {
            result.push_back(a[i]);
            ++i; ++j;
        } else if (a[i] < b[j]) {
            ++i;
        } else {
            ++j;
        }
    }
    return result;
}

std::vector<int> union_lists(const std::vector<int>& a, const std::vector<int>& b) {
    std::vector<int> result;
    size_t i = 0, j = 0;
    while (i < a.size() || j < b.size()) {
        if (j == b.size() || (i < a.size() && a[i] < b[j])) {
            result.push_back(a[i++]);
        } else if (i == a.size() || (j < b.size() && b[j] < a[i])) {
            result.push_back(b[j++]);
        } else {
            result.push_back(a[i++]);
            ++j;
        }
    }

    std::vector<int> unique;
    for (size_t k = 0; k < result.size(); ++k) {
        if (k == 0 || result[k] != result[k - 1]) {
            unique.push_back(result[k]);
        }
    }
    return unique;
}

std::vector<int> difference_lists(const std::vector<int>& a, const std::vector<int>& b) {
    std::vector<int> result;
    size_t i = 0, j = 0;
    while (i < a.size()) {
        if (j < b.size() && b[j] < a[i]) {
            ++j;
        } else if (j < b.size() && b[j] == a[i]) {
            ++i; ++j;
        } else {
            result.push_back(a[i++]);
        }
    }
    return result;
}

// загрузка индекса

struct IndexEntry {
    std::string term;
    std::vector<int> postings;
};

void load_index(const std::string& path, std::vector<IndexEntry>& index) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "Файл индекса не найден: " << path << "\n";
        exit(1);
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        size_t pos = line.find(':');
        if (pos == std::string::npos) continue;
        std::string term = line.substr(0, pos);
        std::string rest = line.substr(pos + 1);

        std::vector<int> docs;
        std::stringstream ss(rest);
        std::string id_str;
        while (std::getline(ss, id_str, ',')) {
            if (!id_str.empty()) {
                docs.push_back(std::stoi(id_str));
            }
        }
        index.push_back({term, std::move(docs)});
    }
}

// бинарный поиск термина в отсортированном индексе

std::vector<int> get_postings(const std::vector<IndexEntry>& index, const std::string& term) {
    size_t left = 0, right = index.size();
    while (left < right) {
        size_t mid = (left + right) / 2;
        const std::string& mid_term = index[mid].term;
        if (mid_term == term) {
            return index[mid].postings;
        } else if (mid_term < term) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    return {};
}

std::string read_env_password() {
    std::ifstream env("../.env");
    std::string line;
    if (std::getline(env, line)) {
        size_t eq = line.find('=');
        if (eq != std::string::npos && line.substr(0, eq) == "DB_PASSWORD") {
            return line.substr(eq + 1);
        }
    }
    std::cerr << "Не найден .env\n";
    exit(1);
}

struct DBConfig {
    std::string host;
    int port;
    std::string database;
    std::string user;
    std::string password;
};

DBConfig load_db_config() {
    DBConfig cfg;
    cfg.password = read_env_password();

    std::ifstream yml("../config.yaml");
    if (!yml.is_open()) {
        std::cerr << "Файл config.yaml не найден\n";
        exit(1);
    }

    bool in_db_block = false;
    std::string line;
    while (std::getline(yml, line)) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t");
        std::string clean_line = line.substr(start, end - start + 1);

        if (clean_line == "db:") {
            in_db_block = true;
            continue;
        }
        if (!in_db_block) continue;

        if (line[0] != ' ' && line[0] != '\t') {
            break;
        }

        size_t colon = clean_line.find(':');
        if (colon == std::string::npos) continue;

        std::string key = clean_line.substr(0, colon);
        std::string val = clean_line.substr(colon + 1);

        size_t vstart = val.find_first_not_of(" \t");
        if (vstart != std::string::npos) {
            val = val.substr(vstart);
        }

        if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
            val = val.substr(1, val.size() - 2);
        }

        if (key == "host") {
            cfg.host = val;
        } else if (key == "port") {
            cfg.port = std::stoi(val);
        } else if (key == "database") {
            cfg.database = val;
        } else if (key == "user") {
            cfg.user = val;
        }
    }

    return cfg;
}

struct DocInfo {
    int id;
    std::string normalized_url;
    std::string title;
};

std::vector<DocInfo> fetch_metadata(const std::vector<int>& doc_ids, const DBConfig& cfg) {
    if (doc_ids.empty()) return {};

    std::ostringstream id_list;
    for (size_t i = 0; i < doc_ids.size(); ++i) {
        if (i > 0) id_list << ",";
        id_list << doc_ids[i];
    }

    std::string query = "SELECT id, normalized_url, title FROM public.documents WHERE id IN (" + id_list.str() + ")";

    std::ostringstream conn_str;
    conn_str << "host=" << cfg.host
             << " port=" << cfg.port
             << " dbname=" << cfg.database
             << " user=" << cfg.user
             << " password=" << cfg.password
             << " client_encoding=UTF8";

    PGconn* conn = PQconnectdb(conn_str.str().c_str());
    if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "Ошибка подключения к бд: " << PQerrorMessage(conn) << "\n";
        PQfinish(conn);
        return {};
    }

    PGresult* res = PQexec(conn, query.c_str());
    std::vector<DocInfo> result;
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        int rows = PQntuples(res);
        for (int i = 0; i < rows; ++i) {
            int id = std::stoi(PQgetvalue(res, i, 0));
            std::string url = PQgetvalue(res, i, 1);
            std::string title = PQgetvalue(res, i, 2);
            if (title.empty()) title = "(без заголовка)";
            result.push_back({id, url, title});
        }
    } else {
        std::cerr << "Ошибка SQL: " << PQerrorMessage(conn) << "\n";
    }

    PQclear(res);
    PQfinish(conn);
    return result;
}

// парсинг запроса

std::string to_lower(const std::string& s) {
    std::string r;
    for (char c : s) {
        if (c >= 'A' && c <= 'Z') r += c + 32;
        else r += c;
    }
    return r;
}

std::vector<int> execute_query(const std::string& raw_query, const std::vector<IndexEntry>& index) {
    std::istringstream iss(raw_query);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok) {
        tokens.push_back(to_lower(tok));
    }

    if (tokens.empty()) return {};

    if (tokens.size() == 1) {
        return get_postings(index, tokens[0]);
    }
    if (tokens.size() == 3) {
        if (tokens[1] == "and") {
            auto a = get_postings(index, tokens[0]);
            auto b = get_postings(index, tokens[2]);
            return intersect_lists(a, b);
        }
        if (tokens[1] == "or") {
            auto a = get_postings(index, tokens[0]);
            auto b = get_postings(index, tokens[2]);
            return union_lists(a, b);
        }
    }
    if (tokens.size() == 4 && tokens[1] == "and" && tokens[2] == "not") {
        auto a = get_postings(index, tokens[0]);
        auto b = get_postings(index, tokens[3]);
        return difference_lists(a, b);
    }

    std::cerr << "Неподдерживаемый запрос.\n";
    return {};
}


int main(int argc, char* argv[]) {
    setup_utf8_console();

    bool ids_only_mode = false;
    if (argc == 2 && std::string(argv[1]) == "--ids-only") {
        ids_only_mode = true;
    } else if (argc > 1) {
        return 1;
    }

    std::cout << "Загрузка индекса.\n";
    std::vector<IndexEntry> index;
    load_index("boolean_index.txt", index);
    DBConfig cfg = load_db_config();

    if (ids_only_mode) {
        std::cout << "Режим: только ID. Введите запрос:\n";
    } else {
        std::cout << "\nВведите запрос:\n";
    }

    std::string query;
    while (std::getline(std::cin, query)) {
        if (query == "exit") break;
        if (query.empty()) continue;

        auto doc_ids = execute_query(query, index);
        if (doc_ids.empty()) {
            if (!ids_only_mode) {
                std::cout << "Ничего не найдено.\n\n";
            }
            if (!ids_only_mode) {
                std::cout << "\nВведите запрос:\n";
            }
            continue;
        }

        if (ids_only_mode) {
            std::cout << "Найдено: " << doc_ids.size() << " документов\n";
            for (int id : doc_ids) {
                std::cout << id << "\n";
            }
            std::cout << "Найдено: " << doc_ids.size() << " документов\n";
            std::cout << "\nВведите запрос:\n";
        } else {
            auto results = fetch_metadata(doc_ids, cfg);
            std::cout << "Найдено: " << results.size() << " документов\n";
            for (const auto& r : results) {
                std::cout << "[id: " << r.id << "] " << r.title << " — " << r.normalized_url << "\n";
            }
            std::cout << "Найдено: " << results.size() << " документов\n";
            std::cout << "\n \n";
            std::cout << "\nВведите запрос:\n";
        }
    }

    return 0;
}