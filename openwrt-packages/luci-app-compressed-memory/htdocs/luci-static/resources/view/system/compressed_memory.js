//
// Copyright (C) 2019-2020 Oever González <notengobattery@gmail.com>
//
// Licensed to the public under the Apache License 2.0.
//

'use strict';
'require form';
'require fs';
'require rpc';

var callInitList;

callInitList = rpc.declare({
	object: 'luci',
	method: 'getInitList',
	params: [ 'name' ],
	expect: { '': {} },
	filter: function(res) {
		for (var k in res)
			return +res[k].enabled;
		return null;
	}
});

function compressors (o) {
	o.value('lzo_rle', '1-5: lzo-rle');
	o.value('lzo',     '2-4: lzo');
	o.value('lz4',     '3-6: lz4');
	o.value('zstd',    '4-2: zstd');
	o.value('deflate', '5-1: deflate');
	o.value('lz4hc',   '6-3: lz4hc');
}

function zpool (o) {
	o.value('zbud',   'zbud (ratio: ~2.00)');
	o.value('z3fold', 'z3fold (ratio: ~3.00)');
}

return L.view.extend({
	load: function() {
		return Promise.all([
			callInitList('zram'),
			callInitList('zswap'),
			L.resolveDefault(fs.stat('/bin/zram'), null),
			L.resolveDefault(fs.stat('/bin/zswap'), null),
			L.resolveDefault(fs.stat('/etc/config/compressed_memory'), null)
		]);
	},
	render: function(loaded) {
		var m, s, o;
		// Don't laugh at me... I don't like JavaScript!
		var zr	= !!loaded[0] & !!loaded[2] & !!loaded[4];
		var zs	= !!loaded[1] & !!loaded[3] & !!loaded[4];

		m = new form.Map('compressed_memory',
			_('Compressed swap disk and cache'),
			_('Configure the compressed memory subsystem, allowing in-memory data compression and deduplication to enhance resource usage, trading CPU cycles for more available RAM.'));

		if (zr) {
			s = m.section(form.NamedSection, 'zram', 'params',
				_('Compressed RAM disk [zram]'),
				_('Configure the compressed RAM disk properties. This section will configure the zram "compressed" block device, which will be used by the kernel as a physical swap disk. <i>If your CPU and RAM are fast or your device does not have external storage, zram is the best alternative to free unused or rarely used memory</i>.'));
			s.anonymous	= true;

			o = s.option(form.Flag, 'enabled', _('Enable a zram disk as a swap area'),
				_('If enabled, a zram disk will be configured and used by the kernel as a swap device.'));
			o.default	= true;
			o.rmempty	= false;

			o = s.option(form.ListValue, 'algorithm', _('Compression algorithm'),
				_('Select the compression algorithm.<ul><li>The first number represents the relative speed; the lower the number, the faster<li>The second number represents the relative compression performance; the lower the number, the more savings</ul>'));
			compressors(o);
			o.default	= 'lzo_rle';
			o.depends('enabled', '1');
			o.rmempty	= false;

			o = s.option(form.Value, 'pool_base', _('Base memory pool'),
				_('Tune the maximum percentage of the total system memory used as the compressed zram block device held in memory, assuming that the compressed data is compressible.'));
			o.datatype	= 'and(ufloat,max(125.00))';
			o.default	= '52.05';
			o.depends('enabled', '1');
			o.rmempty	= false;

			o = s.option(form.Value, 'backing_file', _('Backing storage device'),
				_('Choose a regular file or block device to use when memory is poorly compressible and <b>offers no gain to keep in memory; or when the data is unused for long periods</b>. The kernel will move data away from the compressed in-memory block to this device, effectively freeing it.'));
			o.datatype	= 'or(device,file)';
			o.depends('enabled', '1');
			o.optional	= true;
			o.placeholder	= '/dev/sda1';
			o.rmempty	= false;

			o = s.option(form.Flag, 'advanced', _('Show advanced setup'));
			o.default	= false;
			o.depends('enabled', '1');
			o.rmempty	= false;

			o = s.option(form.Value, 'pool_limit', _('Absolute memory pool limit'),
				_('For calculating the disk\'s size (which is different from the estimated size occupied by it when filled with data, in memory); <b>never</b> size the disk bigger than this hard limit.<br>This limit is a percentage of the total system memory.'));
			o.datatype	= 'and(ufloat,max(480.00))';
			o.default	= '200';
			o.depends('advanced', '1');
			o.rmempty	= false;
		}

		if (zs) {
			s = m.section(form.NamedSection, 'zswap', 'params',
				_('Compressed swap cache [zswap]'),
				_('Configure the compressed swap cache. When used as a cache to a swap device or file, it will reduce the I/O requests to the device or filesystem, allowing faster access to swapped memory areas by keeping them in memory.<br>When used as a cache to zram, it will avoid using the more efficient, yet more expensive, zram block compressor.<br><b>Note that while zram is a compressed block device, this is a compact-page LRU cache, and while zram can work as a stand-alone swap device, zswap always needs a backing swap device, which can be any swap area, including zram</b>.'));
			s.anonymous	= true;

			o = s.option(form.Flag, 'enabled', _('Enable a zswap cache for swap devices'),
				_('Enable a compressed swap cache pool shared by all of the system\'s swap areas.'));
			o.default	= true;
			o.rmempty	= false;

			o = s.option(form.ListValue, 'algorithm', _('Compression algorithm'),
				_('Select the fastest; there is no point in selecting a slower algorithm because the zpool limits compression efficiency.<ul><li>The first number represents the relative speed; the lower the number, the faster<li>The second number means the relative compression performance; the lower the number, the more savings</ul>'));
			compressors(o);
			o.default	= 'lzo_rle';
			o.depends('enabled', '1');
			o.rmempty	= false;

			o = s.option(form.Value, 'pool', _('Maximum memory pool'),
				_('Configure the maximum percentage of the total system memory used as the compressed in-memory LRU cache.<br>It is effective only when zram is not enabled or not managed by this application.'));
			o.datatype	= 'and(ufloat,max(90.00))';
			o.default	= '30.00';
			o.depends('enabled', '1');
			o.rmempty	= false;

			o = s.option(form.ListValue, 'zpool', _('Memory allocator'),
				_('Select the compressed memory allocator. The zpool is a more efficient and faster allocator than the one used by zram, <i>limiting the maximum compression ratio that the compression algorithm can achieve</i>.'));
			zpool(o);
			o.default	= 'z3fold';
			o.depends('enabled', '1');
			o.rmempty	= false;

			o = s.option(form.Flag, 'advanced', _('Show advanced setup'));
			o.default	= false;
			o.depends('enabled', '1');
			o.rmempty	= false;

			o = s.option(form.Value, 'swappiness', _('System swappiness'),
				_('Configure the system\'s tendency to swap unused pages instead of dropping the filesystem cache.<br>If using a compressed filesystem, such as SQUASHFS, UBI, or ZFS, use a higher swappiness because they are often more demanding and slower to read or decompress.<br>The system default swappiness is <b>60%</b>. Values towards 100% will prefer swapping instead of dropping filesystem cache.'));
			o.datatype	= 'and(uinteger,max(100))';
			o.default	= '80';
			o.depends('advanced', '1');
			o.rmempty	= false;

			if (zr) {
				o = s.option(form.ListValue, 'compressor_scale', _('Compressor for zram when using zswap'),
					_('Configure the compression algorithm for zram when zswap is enabled.<br>Since the most requested pages will land in the faster zswap cache, select the most efficient compressor instead of the fastest.<ul><li>The first number represents the relative speed; the lower the number, the faster<li>The second number means the relative compression performance; the lower the number, the more savings</ul>'));
				compressors(o);
				o.default	= 'zstd';
				o.depends('advanced', '1');
				o.rmempty	= false;

				o = s.option(form.Value, 'zswap_scale_pool', _('Scale factor'),
					_('Tune the percentage that will use the zswap pool when zram is enabled. This factor represents the uncompressed data size as a percentage of the zram\'s pool maximum size.'));
				o.datatype	= 'and(ufloat,max(75.00))';
				o.default	= '10.00';
				o.depends('advanced', '1');
				o.rmempty	= false;
			}
		}

		return m.render();
	}
});
