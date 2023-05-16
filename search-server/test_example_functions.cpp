#include "test_example_functions.h"

using namespace std::literals::string_literals;

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint) {
    if (!value) {
        std::cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        std::abort();
    }
}

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };
    // —начала убеждаемс€, что поиск слова, не вход€щего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1, "The server incorrectly finds the document"s);
        //если размер равен 1, можем обратитьс€ к 0 эл-ту
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL_HINT(doc0.id, doc_id, "The server incorrectly finds the document"s);
    }

    // «атем убеждаемс€, что поиск этого же слова, вход€щего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "The server does not exclude stop words"s);
    }
}

void TestCorrectSearchedDocs() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };
    //ѕроверка на добавление документов, изначально 0, после добавлени€ -> увиличиваетс€
    {
        SearchServer server;
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 0, "After creating the server, the number of documents is not equal to 0"s);
        server.AddDocument(43, "dog in the city"s, DocumentStatus::ACTUAL, { 1, 5, 1 });
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 1, "When adding a document, the number of documents does not change"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 2, "When adding a document, the number of documents does not change"s);
    }
    //ѕроверка на пустой запрос
    {
        SearchServer server;
        server.AddDocument(43, "dog in the city"s, DocumentStatus::ACTUAL, { 1, 5, 1 });
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("").empty(), "The server incorrectly returns the result with an empty request"s);
    }
    //ѕроверка на поиск документа
    {
        SearchServer server;
        server.AddDocument(43, "dog in the city"s, DocumentStatus::ACTUAL, { 1, 5, 1 });
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1, "The server returns an incorrect result"s);
        //если размер равен 1, можем обратитьс€ к 0 эл-ту
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL_HINT(doc0.id, doc_id, "The server returns an incorrect result"s);
    }
}

void TestCorrectReturnQueryWithMinusWords() {
    SearchServer server;
    server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(3, "cat in the village"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const auto find_doc = server.FindTopDocuments("cat -village"s);
    ASSERT_EQUAL_HINT(find_doc.size(), 1, "Documents containing negative keywords should not be included in the search result"s);
    //если размер равен 1, можем обратитьс€ к 0 эл-ту => id
    ASSERT_EQUAL_HINT(find_doc[0].id, 2, "Documents containing negative keywords should not be included in the search result"s);
}

void TestMatchingDocs() {
    SearchServer server("in the"s);
    const std::string query = "cat city -dog"s;
    server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(3, "cat in the village"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(4, "sky in the village"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    {
        const auto& [matched_words, status] = server.MatchDocument(query, 1);
        const DocumentStatus res_status = DocumentStatus::ACTUAL;
        ASSERT_HINT(matched_words.empty(), "The server return incorrect result"s);
        ASSERT_EQUAL_HINT(status, res_status, "The server return incorrect status"s);
    }
    {
        const auto& [matched_words, status] = server.MatchDocument(query, 2);
        const std::vector<std::string> res_matched_words = { "cat"s, "city"s };
        const DocumentStatus res_status = DocumentStatus::ACTUAL;
        ASSERT_EQUAL_HINT(matched_words, res_matched_words, "The server return incorrect result"s);
        ASSERT_EQUAL_HINT(status, res_status, "The server return incorrect status"s);
    }
    {
        const auto& [matched_words, status] = server.MatchDocument(query, 3);
        const std::vector<std::string> res_matched_words = { "cat"s };
        const DocumentStatus res_status = DocumentStatus::ACTUAL;
        ASSERT_EQUAL_HINT(matched_words, res_matched_words, "The server return incorrect result"s);
        ASSERT_EQUAL_HINT(status, res_status, "The server return incorrect status"s);
    }
    {
        const auto& [matched_words, status] = server.MatchDocument(query, 4);
        ASSERT_HINT(matched_words.empty(), "The server return incorrect result"s);
        const DocumentStatus res_status = DocumentStatus::ACTUAL;
        ASSERT_EQUAL_HINT(status, res_status, "The server return incorrect status"s);
    }
}

void TestSortedFindDocs() {
    SearchServer server("in the"s);
    const std::string query = "cat city -dog"s;
    server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(3, "cat in the village"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(4, "sky in the village"s, DocumentStatus::ACTUAL, { 1, 2, 3 });

    const std::vector<Document> find_top = server.FindTopDocuments(query);
    for (size_t i = 1; i < find_top.size(); ++i) {
        // сравниваем [ { 1, 0 }, .., { last, last - 1 } ]
        ASSERT_HINT(find_top[i].relevance <= find_top[i - 1].relevance, "The server does not sort documents correctly"s);
    }
}

void TestComputeAverageRating() {

    SearchServer server;
    server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, { 1, 3, 3 });
    server.AddDocument(2, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 4, 3 });
    server.AddDocument(3, "cat in the village"s, DocumentStatus::ACTUAL, {});
    server.AddDocument(4, "sky in the village"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    const std::vector<Document> find_top = server.FindTopDocuments("dog cat sky"s);
    ASSERT_EQUAL_HINT(find_top.size(), 4, "The server does not add documents correctly"s);
    ASSERT_EQUAL_HINT(find_top.at(0).rating, (1 + 3 + 3) / 3, "The server incorrectly counts the rating"s);
    ASSERT_EQUAL_HINT(find_top.at(1).rating, (1 + 1 + 1) / 3, "The server incorrectly counts the rating"s);
    ASSERT_EQUAL_HINT(find_top.at(2).rating, (1 + 4 + 3) / 3, "The server incorrectly counts the rating"s);
    ASSERT_EQUAL_HINT(find_top.at(3).rating, 0, "The server incorrectly counts the rating"s);

}

void TestFilterTopDocsWithPredicate() {
    SearchServer server;
    server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(3, "cat in the village"s, DocumentStatus::ACTUAL, { 1, 2, 1 });
    server.AddDocument(4, "sky in the village"s, DocumentStatus::BANNED, { 1, 1, 1 });
    server.AddDocument(5, "cat in the city for some"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const std::string query = "dog cat sky"s;
    {
        const std::vector<Document> find_top = server.FindTopDocuments(query, [](int document_id, DocumentStatus status, int rating) {return document_id % 2 == 0; });
        for (const Document& doc : find_top) {
            ASSERT_EQUAL_HINT(doc.id % 2, 0, "When filtering by even numbers, the server returns odd numbers"s);
        }
    }
    {
        const std::vector<Document> find_top = server.FindTopDocuments(query, [](int document_id, DocumentStatus status, int rating) {return status == DocumentStatus::BANNED; });
        ASSERT_EQUAL(find_top.size(), 1);
        for (const Document& doc : find_top) {
            const auto& [_, status] = server.MatchDocument(query, doc.id);
            const DocumentStatus res_status = DocumentStatus::BANNED;
            ASSERT_EQUAL_HINT(status, res_status, "The server finds documents with unnecessary statuses."s);
        }
    }
    {
        const std::vector<Document> find_top = server.FindTopDocuments(query, [](int document_id, DocumentStatus status, int rating) {return status == DocumentStatus::BANNED && document_id % 2 != 0; });
        ASSERT_HINT(find_top.empty(), "The server does not filter the found documents correctly. Status + odd"s);
    }
    {
        const std::vector<Document> find_top = server.FindTopDocuments(query, [](int document_id, DocumentStatus status, int rating) {return rating > 1; });
        ASSERT_EQUAL_HINT(find_top.size(), 3, "The server does not filter the found documents correctly"s);
        for (const Document& doc : find_top) {
            ASSERT_HINT(doc.rating > 1, "The server does not filter the found documents correctly. Ratings"s);
        }
    }
}

void TestFindDocsWithStatus() {
    SearchServer server;
    server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "cat in the city"s, DocumentStatus::IRRELEVANT, { 1, 2, 3 });
    server.AddDocument(3, "cat in the village"s, DocumentStatus::REMOVED, { 1, 2, 1 });
    server.AddDocument(4, "sky in the village"s, DocumentStatus::BANNED, { 1, 1, 1 });
    const std::string query = "dog cat sky"s;
    {
        const std::vector<Document> find_top = server.FindTopDocuments(query, [](int document_id, DocumentStatus status, int rating) {return status == DocumentStatus::ACTUAL; });
        ASSERT_EQUAL_HINT(find_top.size(), 1, "The server does not correctly assign the status to documents"s);
        for (const Document& doc : find_top) {
            const auto& [_, status] = server.MatchDocument(query, doc.id);
            const DocumentStatus res_status = DocumentStatus::ACTUAL;
            ASSERT_EQUAL_HINT(status, res_status, "The server finds documents with unnecessary statuses"s);
        }
    }
    {
        const std::vector<Document> find_top = server.FindTopDocuments(query, [](int document_id, DocumentStatus status, int rating) {return status == DocumentStatus::BANNED; });
        ASSERT_EQUAL_HINT(find_top.size(), 1, "The server does not correctly assign the status to documents"s);
        for (const Document& doc : find_top) {
            const auto& [_, status] = server.MatchDocument(query, doc.id);
            const DocumentStatus res_status = DocumentStatus::BANNED;
            ASSERT_EQUAL_HINT(status, res_status, "The server finds documents with unnecessary statuses"s);
        }
    }
    {
        const std::vector<Document> find_top = server.FindTopDocuments(query, [](int document_id, DocumentStatus status, int rating) {return status == DocumentStatus::IRRELEVANT; });
        ASSERT_EQUAL_HINT(find_top.size(), 1, "The server does not correctly assign the status to documents"s);
        for (const Document& doc : find_top) {
            const auto& [_, status] = server.MatchDocument(query, doc.id);
            const DocumentStatus res_status = DocumentStatus::IRRELEVANT;
            ASSERT_EQUAL_HINT(status, res_status, "The server ocuments finds documents with unnecessary statuses"s);
        }
    }
    {
        const std::vector<Document> find_top = server.FindTopDocuments(query, [](int document_id, DocumentStatus status, int rating) {return status == DocumentStatus::REMOVED; });
        ASSERT_EQUAL_HINT(find_top.size(), 1, "The server does not correctly assign the status to documents"s);
        for (const Document& doc : find_top) {
            const auto& [_, status] = server.MatchDocument(query, doc.id);
            const DocumentStatus res_status = DocumentStatus::REMOVED;
            ASSERT_EQUAL_HINT(status, res_status, "The server finds documents with unnecessary statuses"s);
        }
    }
    {
        const std::vector<Document> find_top = server.FindTopDocuments(query, [](int document_id, DocumentStatus status, int rating) {return status == static_cast<DocumentStatus>(6); });
        ASSERT_HINT(find_top.empty(), "The server finds documents with a non-existent status"s);
    }
}

void TestRelevanceTopDocs() {
    SearchServer server;
    server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(3, "cat in the village"s, DocumentStatus::ACTUAL, { 1, 2, 1 });
    server.AddDocument(4, "sky in the village"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    server.AddDocument(5, "sky in the village from house"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    const std::vector<Document> find_top = server.FindTopDocuments("cat city -house"s);
    // цикл по словам =>  relevance +=(term * inv)

    // INV => кол-во документов / в скольки докуах он встречалс€ (cat - 2, city - 2)
    // double inv = log(server.GetDocumentCount() * 1.0 / );

    // TEM => сколько раз слово встречаетс€ в документе / сумма всех слов документа P.S по id документа работаем (2)
    // double term_cat = 1.0 / 4;


    //doc0
    //CAT
    //double rel_cat = log(server.GetDocumentCount() / 2.0) * ((1.0 / 4));
    //CITY
    //double rel_city = log(server.GetDocumentCount() / 2.0) * ((1.0 / 4));

    ASSERT_EQUAL_HINT(find_top.size(), 3, "The server returns an incorrect number of documents"s);

    {
        double res_relevance = log(server.GetDocumentCount() / 2.0) * ((1.0 / 4)) + log(server.GetDocumentCount() / 2.0) * ((1.0 / 4));
        ASSERT_HINT(abs(find_top[0].relevance - res_relevance) < ACCURACY, "the server does not correctly consider relevance"s);
    }

    //doc1
    //CITY
    //double rel_city = log(server.GetDocumentCount() / 2.0) * ((1.0 / 4))
    {
        double res_relevance = log(server.GetDocumentCount() / 2.0) * ((1.0 / 4));
        ASSERT_HINT(abs(find_top[1].relevance - res_relevance) < ACCURACY, "the server does not correctly consider relevance"s);
    }
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestCorrectSearchedDocs);
    RUN_TEST(TestCorrectReturnQueryWithMinusWords);
    RUN_TEST(TestMatchingDocs);
    RUN_TEST(TestSortedFindDocs);
    RUN_TEST(TestComputeAverageRating);
    RUN_TEST(TestFilterTopDocsWithPredicate);
    RUN_TEST(TestFindDocsWithStatus);
    RUN_TEST(TestRelevanceTopDocs);
    // Ќе забудьте вызывать остальные тесты здесь
}