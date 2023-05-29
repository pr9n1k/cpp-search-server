#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) { 
    //список id документов на удаление
    std::set<int> remove_doc_id;
    //список уникальных документов
    std::map<std::set<std::string>, int> unique_docs;
    //Проходим по каждому документу из сервера
    for (const int doc_id : search_server) {
        //все слова документа
        const std::map<std::string, double> doc_words = search_server.GetWordFrequencies(doc_id);
        //уникальные слова
        std::set<std::string> doc_unique_words;
        //добавляем в контейнер уникальные слова
        for (const auto& [word, tf] : doc_words) {
            doc_unique_words.insert(word);
        }
        //Если в списке уникальных документов есть документ с таким же набором уникальных слов
        if (unique_docs.count(doc_unique_words)) {
            //добавляем id этого документа в список на удаление
            remove_doc_id.insert(doc_id);
        }
        else {
            //иначе добавляем в словарь уникальных документов
            unique_docs.insert({ doc_unique_words,doc_id });
        }
    }
    //Удаляем дубликаты
    for (const int id : remove_doc_id) {
        std::cout << std::string("Found duplicate document id ") << id << std::endl;
        search_server.RemoveDocument(id);
    }
}