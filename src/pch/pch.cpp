// clang-format off
//
// **** Expensive headers:
// 104658 ms: /home/sungsik/Projects/core/src/common/types/common_types.h (included 104 times, avg 1006 ms), included via:
//   6x: entrypoint.h command.h request.h beast_utils.h 
//   6x: log.h common.h 
//   4x: messenger.h messenger_core.h log.h common.h 
//   3x: db.h pool.h promise.h debug.h log.h common.h 
//   3x: license.h common.h 
//   3x: common.h 
//   ...
//
// 95005 ms: /home/sungsik/Projects/core/src/common/global_data/global_data_view.h (included 40 times, avg 2375 ms), included via:
//   3x: directory.h 
//   3x: utils.h 
//   2x: fragment_set.h 
//   2x: fragment_set_log.h fragment_set_element.h 
//   1x: fragment_set_element.h 
//   1x: <direct include>
//   ...
//
// 86424 ms: /home/sungsik/Projects/core/src/common/network/messenger.h (included 51 times, avg 1694 ms), included via:
//   5x: configuration.h config.h server.h protocol_handler.h 
//   4x: <direct include>
//   3x: service_registry.h server.h protocol_handler.h 
//   3x: utils.h global_data_view.h service_maintainer.h service_factory.h attached_service.h configuration.h config.h server.h protocol_handler.h 
//   2x: fragment_set_log.h config.h server.h protocol_handler.h 
//   2x: fragment_set.h global_data_view.h service_maintainer.h service_factory.h attached_service.h configuration.h config.h server.h protocol_handler.h 
//   ...
//
// 83088 ms: /home/sungsik/Projects/core/src/entrypoint/http/request.h (included 47 times, avg 1767 ms), included via:
//   6x: entrypoint.h command.h 
//   2x: service.h request_factory.h 
//   1x: abort_multipart.h command.h 
//   1x: module.h command.h 
//   1x: <direct include>
//   1x: put_user_policy.h command.h 
//   ...
//
// 81525 ms: /home/sungsik/Projects/core/src/common/types/address.h (included 107 times, avg 761 ms), included via:
//   6x: entrypoint.h command.h request.h beast_utils.h common_types.h 
//   4x: log.h common.h common_types.h 
//   4x: messenger.h messenger_core.h log.h common.h common_types.h 
//   3x: license.h common.h common_types.h 
//   3x: db.h pool.h promise.h debug.h log.h common.h common_types.h 
//   3x: common.h common_types.h 
//   ...
//
// 77571 ms: /home/sungsik/Projects/core/src/entrypoint/commands/command.h (included 41 times, avg 1891 ms), included via:
//   6x: entrypoint.h 
//   2x: service.h handler.h command_factory.h 
//   1x: abort_multipart.h 
//   1x: module.h 
//   1x: put_user_policy.h 
//   1x: command_factory.h 
//   ...
//
// 66403 ms: /home/sungsik/Projects/core/src/entrypoint/http/beast_utils.h (included 51 times, avg 1302 ms), included via:
//   6x: entrypoint.h command.h request.h 
//   2x: service.h request_factory.h request.h 
//   1x: abort_multipart.h command.h request.h 
//   1x: module.h command.h request.h 
//   1x: put_user_policy.h command.h request.h 
//   1x: aws4_hmac_sha256.h request.h 
//   ...
//
// 64625 ms: /home/sungsik/Projects/core/src/common/network/server.h (included 47 times, avg 1375 ms), included via:
//   5x: configuration.h config.h 
//   3x: service_registry.h 
//   3x: utils.h global_data_view.h service_maintainer.h service_factory.h attached_service.h configuration.h config.h 
//   2x: fragment_set_log.h config.h 
//   2x: fragment_set.h global_data_view.h service_maintainer.h service_factory.h attached_service.h configuration.h config.h 
//   1x: gdv_fixture.h recovery.h service_maintainer.h service_factory.h attached_service.h configuration.h config.h 
//   ...
//
// 59865 ms: /home/sungsik/Projects/core/src/config/configuration.h (included 44 times, avg 1360 ms), included via:
//   5x: <direct include>
//   3x: utils.h global_data_view.h service_maintainer.h service_factory.h attached_service.h 
//   2x: fragment_set.h global_data_view.h service_maintainer.h service_factory.h attached_service.h 
//   2x: fragment_set_log.h fragment_set_element.h global_data_view.h service_maintainer.h service_factory.h attached_service.h 
//   1x: gdv_fixture.h recovery.h service_maintainer.h service_factory.h attached_service.h 
//   1x: service_factory.h attached_service.h 
//   ...
//
// 53859 ms: /home/sungsik/Projects/core/src/common/coroutines/promise.h (included 64 times, avg 841 ms), 
//   4x: <direct include>
//   3x: db.h pool.h 
//   3x: utils.h roundrobin_load_balancer.h storage_group.h coro_util.h 
//   2x: fragment_set.h global_data_view.h ec_get_handler.h storage_group.h coro_util.h 
//   2x: remote_deduplicator.h client.h pool.h 
//   2x: fragment_set_log.h fragment_set_element.h global_data_view.h ec_get_handler.h storage_group.h cor
//   ...
//
// clang-format on
