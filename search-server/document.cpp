#include "document.h"

Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating) {
}


std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << "{ "
        << "document_id = " << document.id << ", "
        << "relevance = " << document.relevance << ", "
        << "rating = " << document.rating << " }";
    return out;
}

std::ostream& operator<<(std::ostream& out, const DocumentStatus& status) {
    switch (status) {
    case DocumentStatus::ACTUAL:
        out << "ACTUAL";
        break;
    case DocumentStatus::BANNED:
        out << "BANNED";
        break;
    case DocumentStatus::IRRELEVANT:
        out << "IRRELEVANT";
        break;
    case DocumentStatus::REMOVED:
        out << "REMOVED";
        break;
    default:
        break;
    }
    return out;
}
