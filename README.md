# openwrt-packages

***小白自用，无技术支持***

国内常用OpenWrt软件包源码合集，每天自动更新，建议使用lean源码


这里luci软件主要是18.06版本用的，不保证19.07可用（19.07可以使用packages-19.07分支，packages分支是18.06使用的）


~~暂时不支持git增量更新（会报错），下次有时间修复下（云编译可以无视）~~


现在可以使用git pull来更新了


## 食用方式（18.06）：
`还是建议按需取用，不然碰到依赖问题不太好解决`
1. 先cd进package目录，然后执行
```bash
 git clone https://github.com/hzjflying123/openwrt-packages
```
2. 或者添加下面代码到feeds.conf.default文件
```bash
 src-git hzjflying123_packages https://github.com/hzjflying123/openwrt-packages
```
3. 先cd进package目录，然后执行
```bash
 svn co https://github.com/hzjflying123/openwrt-packages/branches/packages
```


