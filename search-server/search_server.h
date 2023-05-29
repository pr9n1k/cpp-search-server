#pragma once
#include "string_processing.h"
#include "document.h"
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <set>
#include <map>

const int MAX_RESULT_DOCUMENT_COUNT = 5;


class SearchServer {
public:
    template <typename StringContainer>
    SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
        if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument("Some of stop words are invalid");
        }
    }
    //Конструктор со стоп словами
    explicit SearchServer(const std::string& stop_words_text);
    //конструктор по умолчанию
    explicit SearchServer();
    //Добавлени документа на сервер
    void AddDocument(int document_id, const std::string& document, DocumentStatus status,
        const std::vector<int>& ratings);

    //Поиск документов по условию
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query,
        DocumentPredicate document_predicate) const;
    //Поиск по статусу
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;
    //Поиск без дополнительных параметров
    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;
    //Получение кол-ва документов
    size_t GetDocumentCount() const;
    //int GetDocumentId(int index) const;
    //Получить вектор слов документа и его статус, передав ID-документа
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query,
        int document_id) const;
    //Получить итератор на начало вектора id-шников(document_ids_)
    std::set<int>::const_iterator begin() const;
    //Получить итератор на конец вектора id-шников(document_ids_)
    std::set<int>::const_iterator end() const;

    //Получить список ключ-значение из слова и его частотой встречаемости в документа, ID которого передаем в параметре
    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

    //Удалить документ
    void RemoveDocument(int document_id);

private:
    //Структура данных документа - рейтинг + стстус
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    //структура запроса - поисковые слова + стоп слова
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };
    //стркутура парсинга слова - слово + минус слово? + стоп слово?
    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };
    //Стоп слова
    const std::set<std::string> stop_words_;
    //слово, {id_doc, как часто встречается слово(кол-во раз / кол-во слов в запросе)}
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    //id-документа, {рейтинг, статус}
    std::map<int, DocumentData> documents_;
    //список id-документов
    std::set<int> document_ids_;
    //id-документа, {слово, как часто встречается слово(кол-во раз / кол-во слов в запросе)}
    std::map<int, std::map<std::string, double>> id_document_to_word_freqs_;

    //Проверка на стоп слово
    bool IsStopWord(const std::string& word) const;
    //Проверка на валидность слова
    static bool IsValidWord(const std::string& word);
    //парсинг строки в ветктор слов
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;
    //Вовзращает средний рейтинг, переданного вектора рейтинга
    static int ComputeAverageRating(const std::vector<int>& ratings);
    //Проверяем слово на корректность, стоп-слово и на минус-плюс слово
    QueryWord ParseQueryWord(const std::string& text) const;
    //Парсим строку в структуру Query(минус и плюс слова)
    Query ParseQuery(const std::string& text) const;
    //Получаем частоту обращения слова к документу
    double ComputeWordInverseDocumentFreq(const std::string& word) const;
    //Поиск всех документов по запросу и условию
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const;
};


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query,
    DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
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

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
    DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

