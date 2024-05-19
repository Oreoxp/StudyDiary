#include "tordexhttp.h"

tordex::http::http()
{
}

tordex::http::~http()
{
	stop();
}

bool tordex::http::download_file(const std::string& url, http_request* request)
{
    if (request) {
        if (request->create(url)) {
            m_requests.push_back(request);
            return true;
        }
    }
    return false;
}

void tordex::http::stop()
{
    for (auto& request : m_requests) {
        request->cancel();
    }
    m_requests.clear();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

tordex::http_request::http_request() {
}

tordex::http_request::~http_request() {
	cancel();
}

bool tordex::http_request::create(std::string url) {
	m_url	= url;
    if(!m_client) {
        m_client = std::make_shared<httplib::Client>(url);
    }
    auto res = m_client->Get(url.c_str(), [this](const char* data, size_t size) {
        this->OnData(data, size, m_downloaded_length += size, m_content_length);
        return true;
    });

    if (!res) {
        OnFinish(-1, "Request failed");
        return false;
    }

    m_status = res->status;
    m_response_body = res->body;

    OnFinish(m_status, "");
	return true;
}

void tordex::http_request::cancel()
{
	if(m_client) {
		m_client->stop();
	}
}