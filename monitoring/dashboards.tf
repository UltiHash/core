resource "grafana_dashboard" "hardwaremonitoring" {
   provider = grafana.base

   for_each    = fileset("${path.module}/dashboards", "*.json")
   config_json = file("${path.module}/dashboards/${each.key}")
   folder      = grafana_folder.HardwareMonitoring.id
}
