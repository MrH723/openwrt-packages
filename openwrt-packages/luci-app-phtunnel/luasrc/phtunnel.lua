module("luci.phtunnel", package.seeall)
require("luci.sys")
require("luci.json")

local function write_log(log)
	luci.sys.exec("logger -t phtunnel_web '" .. string.gsub(log, "'", "") .. "'") --用gsub去掉单引号
end

function write_info_log(log)
	write_log("[info] " .. log)
end

function write_err_log(log)
	write_log("[err] " .. log)
end

function convert_status(status)
	if status == 0 then
		return "offline"
	elseif status == 1 then
		return "online"
	elseif status == 2 then
		return "loginning"
	elseif status == 3 then
		return "retry"
	else
		return "unknown"
	end
end

--获取基本信息
function get_base_info()
	local cmd = "wget -O - -q http://127.0.0.1:16062/ora_service/getsn"
	write_info_log("command for get base info: " .. cmd)

	local wget_result = luci.sys.exec(cmd)
	if wget_result then
		local result = luci.json.decode(wget_result)
		if type(result) == "table" and result.result_code == 0 and type(result.data) == "table" then
			return result.result_code, result.data.device_sn, result.data.device_sn_pwd, result.data.status, result.data.public_ip
		else
			write_err_log("get base info failed : result=(" .. wget_result .. ")")
		end
	else
		write_err_log("get base info failed : wget")
	end
end

--获取登录后帐号信息
function get_login_info()
	local cmd = "wget -O - -q http://127.0.0.1:16062/ora_service/getmgrurl"
	write_info_log("command for get login info: " .. cmd)

	local wget_result = luci.sys.exec(cmd)
	if wget_result then
		local result = luci.json.decode(wget_result)
		if type(result) == "table" and result.result_code == 0 and type(result.data) == "table" then
			return result.result_code, result.data.url, result.data.account
		else
			write_err_log("get login info failed : result=(" .. wget_result .. ")")
		end
	else
		write_err_log("get login info failed : wget")
	end
end

--获取扫描信息
function get_qrimage(sn, pwd)
	local post_json = { sn = sn, password = pwd }
	local post_file = "/tmp/phtunnel_qrimg_post"
	local f = io.open(post_file, "w")
	if f then
		f:write(luci.json.encode(post_json))
		f:close()

		local cmd = "wget -O - -q --post-file '" .. post_file .. "' --header 'Content-Type: application/json' --no-check-certificate 'https://hsk-api.oray.com/devices/qrcode'"
		write_info_log("command for get qrcode image : " .. cmd)

		local post_data = ""
		local wget_result = luci.sys.exec(cmd)
		os.remove(post_file)
		if wget_result then
			local result = luci.json.decode(wget_result)
			if type(result) == "table" then
				return result.qrcode, result.qrcodeimg, result.ttl
			else
				write_err_log("get qrcode image failed : result=(" .. wget_result .. ")")
			end
		end
	else
		write_log("get qrcode image failed : wget")
	end
end

--解绑
function unbind_account(sn, pwd)
	local post_json = { sn = sn, password = pwd }
	local post_file = "/tmp/phtunnel_unbind_post"
	local f = io.open(post_file, "w")
	if f then
		f:write(luci.json.encode(post_json))
		f:close()

		local cmd = "wget -O - -q --post-file '" .. post_file .. "' --header 'Content-Type: application/json' --no-check-certificate 'https://hsk-api.oray.com/devices/unbinding'"
		write_info_log("command for get unbind : " .. cmd)

		local post_data = ""
		local wget_result = luci.sys.exec(cmd)
		--os.remove(post_file)
		if wget_result then
			return true
		end
	else
		write_log("unbind account failed : wget")
	end
end

