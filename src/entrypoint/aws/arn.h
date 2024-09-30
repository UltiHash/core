#ifndef CORE_ENTRYPOINT_AWS_ARN_H
#define CORE_ENTRYPOINT_AWS_ARN_H

#include <string>
#include <vector>

namespace uh::cluster::ep::aws {

class arn {
public:
    arn(std::string service, std::string account, std::string resource_id);
    arn(std::string text);

    const std::string& service() const;
    void service(const std::string& value);

    const std::string& account() const;
    void account(const std::string& value);

    const std::string& resource_id() const;
    void resource_id(const std::string& value);

    std::string to_string() const;

private:
    std::vector<std::string> m_fields;
};

} // namespace uh::cluster::ep::aws

#endif
