terraform {
   required_providers {
      grafana = {
         source  = "grafana/grafana"
         version = "1.38.0"
      }
   }
}

provider "grafana" {
   alias = "base"

   url   = "http://193.240.203.196:3001/"
   auth  = "eyJrIjoicElhVXA5bUZWVE1hT1VxYVZ4YUF2R0xXeERtcXpBNkIiLCJuIjoiZ2l0aHViX2FjdGlvbnMiLCJpZCI6MX0="
}
