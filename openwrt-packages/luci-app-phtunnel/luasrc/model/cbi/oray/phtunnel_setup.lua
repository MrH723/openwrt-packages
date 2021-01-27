m = Map("phtunnel", translate("setup"))
m.reset = true

local s = m:section(NamedSection, "base", "base", "Base setup")

enabled = s:option(Flag, "enabled", translate("Enabled"))
app_id = s:option(Value, "app_id", translate("App ID"))
app_key = s:option(Value, "app_key", translate("App Key"))
--server = s:option(Value, "server", translate("Server Address"))

app_id:depends("enabled", "1")
app_key:depends("enabled", "1")
--server:depends("enabled", "1")

enabled.rmempty = false
app_id.rmempty = true
app_key.rmempty = true
--server.rmempty = true

local apply = luci.http.formvalue("cbi.apply")
if apply then   
	io.popen("/etc/init.d/phtunnel restart")
end 

return m

