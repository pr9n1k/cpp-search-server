#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;


template<typename T>
void Print(ostream& out, const T& container) {
    bool firstStep = true;

    for (const auto& element : container) {
        if (!firstStep) {
            out << ", ";
        }
        out << element;
        firstStep = false;
    }
}

template<typename T>
ostream& operator<<(ostream& out, const vector<T>& container) {
    cout << "[";
    Print(out, container);
    cout << "]";
    return out;
}

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    template<typename Func>
    vector<Document> FindTopDocuments(const string& raw_query, Func filter) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, filter);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }


    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus doc_status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [doc_status](int document_id, DocumentStatus status, int rating) { return status == doc_status; });
    }


    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    bool DefaultFilter(int document_id, DocumentStatus status, int rating) {
        return status == DocumentStatus::ACTUAL;
    };

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
    template<typename T>
    vector<Document> FindAllDocuments(const Query& query, T filter) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (filter(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

// Реализация макросов 

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T>
void RunTestImpl(const T& func, const string& name_func) {
    func();
    cerr << name_func << " OK" << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func) 

// Конец реализации макросов

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty(), "The list should be empty"s);
    }
}

/*
Разместите код остальных тестов здесь
*/
//Тест на корректный поиск документов
void TestCorrectSearchedDocs() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    //Проверка на пустой запрос
    {
        SearchServer server;
        server.AddDocument(43, "dog in the city"s, DocumentStatus::ACTUAL, { 1, 5, 1 });
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments(""s).empty(), "The list should be empty"s);
    }
    //Проверка на поиск документа
    {
        SearchServer server;
        server.AddDocument(43, "dog in the city"s, DocumentStatus::ACTUAL, { 1, 5, 1 });
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
}

//Документы содержащие минус слова не должны включаться в результат поиска
void TestCorrectReturnQueryWithMinusWords() {
    SearchServer server;
    server.AddDocument(1, "dog in the city", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "cat in the city", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(3, "cat in the village", DocumentStatus::ACTUAL, { 1,2,3 });
    const auto find_doc = server.FindTopDocuments("cat -village");
    ASSERT_EQUAL(find_doc.size(), 1);
    ASSERT_EQUAL(find_doc[0].id, 2);
}

void TestMatchingDocs() {
    SearchServer server;
    const string query = "cat city -dog"s;
    server.SetStopWords("in the");
    server.AddDocument(1, "dog in the city", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "cat in the city", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(3, "cat in the village", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(4, "sky in the village", DocumentStatus::ACTUAL, { 1,2,3 });
    {
        const tuple<vector<string>, DocumentStatus> matching_doc = server.MatchDocument(query, 1);
        ASSERT(get<0>(matching_doc).empty());
    }
    {
        const tuple<vector<string>, DocumentStatus> matching_doc = server.MatchDocument(query, 2);
        const vector<string> res = { "cat"s,"city"s };
        ASSERT_EQUAL(get<0>(matching_doc), res);
    }
    {
        const tuple<vector<string>, DocumentStatus> matching_doc = server.MatchDocument(query, 3);
        const vector<string> res = { "cat"s };
        ASSERT_EQUAL(get<0>(matching_doc), res);
    }
    {
        const tuple<vector<string>, DocumentStatus> matching_doc = server.MatchDocument(query, 4);
        ASSERT(get<0>(matching_doc).empty());
    }
}

void TestSortedFindDocs() {
    SearchServer server;
    const string query = "cat city -dog"s;
    server.SetStopWords("in the");
    server.AddDocument(1, "dog in the city", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "cat in the city", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(3, "cat in the village", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(4, "sky in the village", DocumentStatus::ACTUAL, { 1,2,3 });

    const vector<Document> find_top = server.FindTopDocuments(query);
    double relevance = 1.1;
    for (const Document& d : find_top) {
        ASSERT(d.relevance < relevance);
        relevance = d.relevance;
    }
}

void TestComputeAverageRating() {

    SearchServer server;
    server.AddDocument(1, "dog in the city", DocumentStatus::ACTUAL, { 1,3,3 });
    server.AddDocument(2, "cat in the city", DocumentStatus::ACTUAL, { 1,4,3 });
    server.AddDocument(3, "cat in the village", DocumentStatus::ACTUAL, {});
    server.AddDocument(4, "sky in the village", DocumentStatus::ACTUAL, { 1,1,1 });
    const vector<Document> find_top = server.FindTopDocuments("dog cat sky"s);
    ASSERT_EQUAL(find_top.at(0).rating, 2);
    ASSERT_EQUAL(find_top.at(1).rating, 1);
    ASSERT_EQUAL(find_top.at(2).rating, 2);
    ASSERT_EQUAL(find_top.at(3).rating, 0);

}

void TestFilterTopDocsWithPredicat() {
    SearchServer server;
    server.AddDocument(1, "dog in the city", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "cat in the city", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(3, "cat in the village", DocumentStatus::ACTUAL, { 1,2,1 });
    server.AddDocument(4, "sky in the village", DocumentStatus::BANNED, { 1,1,1 });
    server.AddDocument(5, "cat in the city for some", DocumentStatus::ACTUAL, { 1,2,3 });
    {
        const vector<Document> find_top = server.FindTopDocuments("dog cat sky"s, [](int document_id, DocumentStatus status, int rating) {return document_id % 2 == 0; });
        for (const Document& doc : find_top) {
            ASSERT_EQUAL(doc.id % 2, 0);
        }
    }
    {
        const vector<Document> find_top = server.FindTopDocuments("dog cat sky"s, [](int document_id, DocumentStatus status, int rating) {return status == DocumentStatus::BANNED; });
        ASSERT_EQUAL(find_top.size(), 1);
    }
    {
        const vector<Document> find_top = server.FindTopDocuments("dog cat sky"s, [](int document_id, DocumentStatus status, int rating) {return status == DocumentStatus::BANNED && document_id % 2 != 0; });
        ASSERT(find_top.empty());
    }
    {
        const vector<Document> find_top = server.FindTopDocuments("dog cat sky"s, [](int document_id, DocumentStatus status, int rating) {return rating > 1; });
        ASSERT_EQUAL(find_top.size(), 3);
    }
}

void TestFindDocsWithStatus() {
    SearchServer server;
    server.AddDocument(1, "dog in the city", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "cat in the city", DocumentStatus::IRRELEVANT, { 1,2,3 });
    server.AddDocument(3, "cat in the village", DocumentStatus::REMOVED, { 1,2,1 });
    server.AddDocument(4, "sky in the village", DocumentStatus::BANNED, { 1,1,1 });
    {
        const vector<Document> find_top = server.FindTopDocuments("dog cat sky"s, [](int document_id, DocumentStatus status, int rating) {return status == DocumentStatus::ACTUAL; });
        ASSERT_EQUAL(find_top.size(), 1);
    }
    {
        const vector<Document> find_top = server.FindTopDocuments("dog cat sky"s, [](int document_id, DocumentStatus status, int rating) {return status == DocumentStatus::BANNED; });
        ASSERT_EQUAL(find_top.size(), 1);
    }
    {
        const vector<Document> find_top = server.FindTopDocuments("dog cat sky"s, [](int document_id, DocumentStatus status, int rating) {return status == DocumentStatus::IRRELEVANT; });
        ASSERT_EQUAL(find_top.size(), 1);
    }
    {
        const vector<Document> find_top = server.FindTopDocuments("dog cat sky"s, [](int document_id, DocumentStatus status, int rating) {return status == DocumentStatus::REMOVED; });
        ASSERT_EQUAL(find_top.size(), 1);
    }
}

void TestRelevanceTopDocs() {
    SearchServer server;
    server.AddDocument(1, "dog in the city", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "cat in the city", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(3, "cat in the village", DocumentStatus::ACTUAL, { 1,2,1 });
    server.AddDocument(4, "sky in the village", DocumentStatus::ACTUAL, { 1,1,1 });
    server.AddDocument(5, "sky in the village from house", DocumentStatus::ACTUAL, { 1,1,1 });
    const vector<Document> find_top = server.FindTopDocuments("cat city - house"s);
    //assert(find_top[0].relevance == 0.458145); abs(lhs.relevance - rhs.relevance) < 1e-6
    ASSERT(abs(find_top[0].relevance - 0.458145) < 1e-6);
}
/*
Разместите код остальных тестов здесь КОНЕЦ
*/

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestCorrectSearchedDocs);
    RUN_TEST(TestCorrectReturnQueryWithMinusWords);
    RUN_TEST(TestMatchingDocs);
    RUN_TEST(TestSortedFindDocs);
    RUN_TEST(TestComputeAverageRating);
    RUN_TEST(TestFilterTopDocsWithPredicat);
    RUN_TEST(TestFindDocsWithStatus);
    RUN_TEST(TestRelevanceTopDocs);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s << endl;
}
int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}