
#include <stdio.h>

#include "simple-cgi.h"

void print_style_sheet(void)
{
	printf("<style type=\"text/css\">\n");
	printf("body {\n");
	printf("\tfont-family: Arial;\n");
	printf("\tpadding: 10px 10px 10px 10px;\n");
	printf("\tfont-size: 12px;\n");
	printf("}\n");
	printf("h1 {\n");
	printf("\tcolor: #cc0000;\n");
	printf("\tfont-size: 22px;\n");
	printf("\tfont-weight: normal;\n");
	printf("}\n");
	printf("h2 {\n");
	printf("\tfont-size: 18px;\n");
	printf("\tfont-weight: normal;\n");
	printf("}\n");
	printf("</style>\n");
}

int main(void)
{
	int ret;

	printf("Content-Type:text/html\n\n");

	printf("<html>\n\n");
	printf("<head>\n");
	printf("<title>SIMPLE-CGI TEST</title>\n");
	print_style_sheet();
	printf("</head>\n\n");
	printf("<body>\n\n");
	printf("<h1>SIMPLE-CGI TEST</h1>\n\n");

	ret = cgi_query_process();
	if (ret < 0) {
		printf("<span style=\"color: red\">cgi_query_process() error</span>\n");
		return 0;
	}

	printf("<h2>CGI Environment Variables</h2>\n\n");

	printf("AUTH_TYPE: [%s]<br />\n", cgi_get_auth_type());
	printf("Content length [%d]*<br />\n", cgi_get_content_length());
	printf("CONTENT_TYPE: [%s]<br />\n", cgi_get_content_type());	
	printf("GATEWAY_INTERFACE: [%s]<br />\n", cgi_get_gateway_interface());
	printf("PATH_INFO: [%s]<br />\n", cgi_get_path_info());
	printf("PATH_TRANSLATED: [%s]<br />\n", cgi_get_path_translated());
	printf("QUERY_STRING: [%s]<br />\n", cgi_get_query_string());
	printf("REMOTE_ADDR: [%s]<br />\n", cgi_get_remote_addr());
	printf("REMOTE_HOST: [%s]<br />\n", cgi_get_remote_host());
	printf("REMOTE_IDENT: [%s]<br />\n", cgi_get_remote_ident());
	printf("REMOTE_USER: [%s]<br />\n", cgi_get_remote_user());
	printf("REQUEST_METHOD: [%s]<br />\n", cgi_get_request_method());
	printf("Script name: [%s]*<br />\n", cgi_get_script_name());
	printf("SCRIPT_PATH: [%s]<br />\n", cgi_get_script_path());
	printf("SERVER_NAME: [%s]<br />\n", cgi_get_server_name());						
	printf("SERVER_PORT: [%s]<br />\n", cgi_get_server_port());
	printf("SERVER_PROTOCOL: [%s]<br />\n", cgi_get_server_protocol());
	printf("SERVER_SOFTWARE: [%s]<br />\n", cgi_get_server_software());

	printf("HTTP_ACCEPT: [%s]<br />\n", cgi_get_http_accept());						
	printf("HTTP_USER_AGENT: [%s]<br />\n", cgi_get_http_user_agent());
	printf("HTTP_REFERER: [%s]<br />\n", cgi_get_http_refferer());
	printf("HTTP_COOKIE: [%s]<br />\n", cgi_get_http_cookie());

	printf("<h2>HTML Query Parameters</h2>\n\n");

	printf("<h3>Query values dump</h3>\n\n");
	cgi_query_dump_html();

	printf("<h3>cgi_query_get_val() test</h3>\n\n");
	printf("submit (Submit button value): [%s]<br />\n", cgi_query_get_val("submit"));

	printf("\n</body>\n\n");
	printf("</html>\n");

	cgi_query_free();

	return 0;
}
