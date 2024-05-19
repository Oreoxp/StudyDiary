#pragma once
#include "httplib.h"
#include <stdlib.h>
#include <vector>
#include <string>

namespace tordex
{
	class http_request
	{
		friend class http;
	protected:
		std::string m_url;
		LONG m_refCount = 1;
		std::shared_ptr<httplib::Client> m_client;
		ULONG64 m_content_length;
		ULONG64 m_downloaded_length;
		int m_status;
		httplib::Headers m_headers;
		std::string m_response_body;
	public:
		http_request();
		virtual ~http_request();

		virtual void OnFinish(int status, const std::string& errorMsg) = 0;
		virtual void OnData(const char* data, size_t len, size_t downloaded, size_t total) = 0;
		virtual void OnHeadersReady();

		bool	create(std::string url);
		void	cancel();
		void	add_ref();
		void	release();

	protected:
		static void* read_callback(const char* data, size_t size, size_t nmemb, void* userdata);
		static void* write_callback(const char* data, size_t size, size_t nmemb, void* userdata);
	};

	class http
	{
        std::vector<http_request*> m_requests;

    public:
        http();
        ~http();

        bool download_file(const std::string& url, http_request* request);
        void stop();
	};
}