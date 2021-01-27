module("luci.controller.oray.phtunnel", package.seeall)

function index()
	entry({"admin", "oray"}, alias("admin", "oray", "phtunnel"), _("Oray"), 50).index = true
	entry({"admin", "oray", "phtunnel"}, alias("admin", "oray", "phtunnel", "setup"), _("Phtunnel"))
	entry({"admin", "oray", "phtunnel", "setup"}, cbi("oray/phtunnel_setup"), _("Setup"), 1).leaf = true
	entry({"admin", "oray", "phtunnel", "status"}, template("oray/phtunnel_status"), _("Status"), 2).leaf = true
	entry({"admin", "oray", "phtunnel", "log"}, template("oray/phtunnel_log"), _("Log"), 3).leaf = true

	local node = entry({"admin", "oray", "phtunnel", "inner_status"}, template("oray/phtunnel_inner_status"), _("Inner Status"), 4)
	node.leaf = true
	node.hidden = true

	node = entry({"admin", "oray", "phtunnel", "log_off"}, template("oray/phtunnel_log_off"), _("Log Off"), 5)
	node.leaf = true
	node.hidden = true
end

