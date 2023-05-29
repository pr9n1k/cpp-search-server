#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) { 
    //������ id ���������� �� ��������
    std::set<int> remove_doc_id;
    //������ ���������� ����������
    std::map<std::set<std::string>, int> unique_docs;
    //�������� �� ������� ��������� �� �������
    for (const int doc_id : search_server) {
        //��� ����� ���������
        const std::map<std::string, double> doc_words = search_server.GetWordFrequencies(doc_id);
        //���������� �����
        std::set<std::string> doc_unique_words;
        //��������� � ��������� ���������� �����
        for (const auto& [word, tf] : doc_words) {
            doc_unique_words.insert(word);
        }
        //���� � ������ ���������� ���������� ���� �������� � ����� �� ������� ���������� ����
        if (unique_docs.count(doc_unique_words)) {
            //��������� id ����� ��������� � ������ �� ��������
            remove_doc_id.insert(doc_id);
        }
        else {
            //����� ��������� � ������� ���������� ����������
            unique_docs.insert({ doc_unique_words,doc_id });
        }
    }
    //������� ���������
    for (const int id : remove_doc_id) {
        std::cout << std::string("Found duplicate document id ") << id << std::endl;
        search_server.RemoveDocument(id);
    }
}