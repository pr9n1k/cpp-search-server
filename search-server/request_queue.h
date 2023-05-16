#pragma once
#include <deque>
#include <vector>
#include "search_server.h"
#include "document.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    //������� "������" ��� ���� ������� ������, ����� ��������� ���������� ��� ����� ����������
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        // �������� ����������
        const auto& res = search_server_.FindTopDocuments(raw_query, document_predicate);
        AddRequest(static_cast<int>(res.size()));
        return res;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        // ����������, ��� ������ ���� � ���������
        //���-�� ������������ �� ������� �� ������
        int count_results;
        //� ����� ����� ��� ��������� ������
        size_t time;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    size_t current_time_;
    const SearchServer& search_server_;
    int size_empty_requests_;

    void AddRequest(int result);
};
