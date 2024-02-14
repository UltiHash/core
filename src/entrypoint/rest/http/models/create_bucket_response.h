#pragma once

#include "entrypoint/rest/http/http_request.h"
#include "entrypoint/rest/http/http_response.h"

namespace uh::cluster::rest::http::model {

class create_object_response : public http_response {
  public:
    explicit create_object_response(const http_request&);
    create_object_response(const http_request&,
                           http::response<http::string_body>);

    [[nodiscard]] const http::response<http::string_body>&
    get_response_specific_object() override;

  private:
    bool m_locationHasBeenSet = false;
    std::string m_location;

    bool m_requestIdHasBeenSet = false;
    std::string m_requestId;
};

} // namespace uh::cluster::rest::http::model
