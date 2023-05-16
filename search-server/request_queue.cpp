#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
    : current_time_(0)
    , search_server_(search_server)
    , size_empty_requests_(0)
{
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    const auto& res = search_server_.FindTopDocuments(raw_query, status);
    AddRequest(static_cast<int>(res.size()));
    return res;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    const auto& res = search_server_.FindTopDocuments(raw_query);
    AddRequest(static_cast<int>(res.size()));
    return res;
}

int RequestQueue::GetNoResultRequests() const {
    return size_empty_requests_;
}

void RequestQueue::AddRequest(int result) {
    ++current_time_;
    while (!requests_.empty() && min_in_day_ <= current_time_ - requests_.front().time) {
        if (!requests_.front().count_results) {
            --size_empty_requests_;
        }
        requests_.pop_front();
    }

    requests_.push_back({ result, current_time_ });
    if (!result) {
        ++size_empty_requests_;
    }
}