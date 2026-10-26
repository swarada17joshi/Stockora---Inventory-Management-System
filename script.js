const $ = s => document.querySelector(s);
const esc = s => String(s ?? "").replace(/[&<>"']/g, c => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
const r1 = n => Math.round(n * 10) / 10;
const addDays = n => { const d = new Date(); d.setDate(d.getDate() + Math.ceil(n)); return d.toISOString().slice(0, 10); };
const ui = { q: "", status: "all", cat: "all", sort: "urgency", rt: "need", tab: "appearance" };
let cats = [], anim = true;

async function api(url, method = "GET", body) {
  const r = await fetch(url, { method, headers: { "Content-Type": "application/json" }, body: body ? JSON.stringify(body) : undefined });
  let d = null; try { d = await r.json(); } catch (e) {}
  if (!r.ok) { const er = new Error((d && d.message) || "Something went wrong."); er.data = d; throw er; }
  return d;
}
const badge = p => ({ out: '<span class="badge b-out">Out of Stock</span>', low: '<span class="badge b-low">Low Stock</span>', ok: '<span class="badge b-ok">In Stock</span>' }[p.status]);
function toast(m, bad) { const t = $("#toast"); t.textContent = m; t.className = "show" + (bad ? " bad" : ""); clearTimeout(toast.h); toast.h = setTimeout(() => t.className = "", 2600); }
function openM(h) { $("#dlg").innerHTML = h; $("#modal").classList.add("open"); const i = $("#dlg input,#dlg select"); if (i) i.focus(); }
const closeM = () => $("#modal").classList.remove("open");

/* ---------- views ---------- */
async function overview() {
  const d = await api("/api/overview"), c = d.counts, n = d.total || 1;
  return `<div class="head"><div><h1>Overview</h1><p class="sub">Your stock at a glance.</p></div></div>
  <div class="grid kpis">${[["Total products", d.total, "all", "inventory"], ["Low-stock items", d.lowStock, "lowout", "inventory"], ["Items needing action", d.needAction, "", "restock", "need"], ["Pending orders", d.pending, "", "restock", "prog"]]
    .map(k => `<button class="kpi" data-kpi="${k[2]}" data-to="${k[3]}" data-rt="${k[4] || ""}"><span>${k[0]}</span><b>${k[1]}</b></button>`).join("")}</div>
  <div class="grid two"><div class="card"><h3>Stock status</h3>${[["ok", "In Stock", "var(--teal)"], ["low", "Low Stock", "var(--amber)"], ["out", "Out of Stock", "var(--red)"]]
    .map(([k, l, col]) => `<div class="lv"><small><span>${l}</span><b>${c[k]}</b></small><div class="track"><div class="fill" style="width:${c[k] / n * 100}%;background:${col}"></div></div></div>`).join("")}</div>
  <div class="card"><h3>Needs attention</h3>${d.attention.length ? d.attention.map(x => `<div class="row"><div><b>${esc(x.name)}</b><p>${x.stock} available${x.hasOpenOrder ? " · order in progress" : ""}</p></div>${badge(x)}</div>`).join("") + '<div class="foot"><a class="btn ghost sm" href="#restock">Go to Restock Actions</a></div>' : '<div class="empty">Everything is well stocked.</div>'}</div></div>`;
}

async function inventory() {
  cats = [...new Set((await api("/api/products")).map(p => p.category))].sort();
  const o = (v, l, s) => `<option value="${esc(v)}"${s ? " selected" : ""}>${esc(l)}</option>`;
  return `<div class="head"><div><h1>Inventory</h1><p class="sub">Add, update and track your products.</p></div><button class="btn" data-act="add">Add product</button></div>
  <div class="card"><div class="tools"><input id="q" type="search" placeholder="Search by name or ID" value="${esc(ui.q)}" aria-label="Search">
  <select id="fs" aria-label="Status">${o("all", "All statuses", ui.status === "all")}${o("ok", "In Stock", ui.status === "ok")}${o("low", "Low Stock", ui.status === "low")}${o("out", "Out of Stock", ui.status === "out")}${o("lowout", "Low or out", ui.status === "lowout")}</select>
  <select id="fc" aria-label="Category">${o("all", "All categories", ui.cat === "all")}${cats.map(c => o(c, c, ui.cat === c)).join("")}</select>
  <select id="so" aria-label="Sort">${o("urgency", "Sort: needs ordering first", ui.sort === "urgency")}${o("name", "Sort: name", ui.sort === "name")}${o("stock", "Sort: quantity", ui.sort === "stock")}${o("demand", "Sort: demand", ui.sort === "demand")}</select></div>
  <div class="tw" id="tbl">${await table()}</div></div>`;
}
async function table() {
  const p = new URLSearchParams({ q: ui.q, status: ui.status, category: ui.cat, sort: ui.sort }), a = await api("/api/products?" + p);
  if (!a.length) return '<div class="empty">No products found. Try a different search or add a product.</div>';
  return `<table><thead><tr><th>Product</th><th>ID</th><th>Category</th><th>Available</th><th>Demand / day</th><th>Status</th><th></th></tr></thead><tbody>${a.map(x => `<tr><td><b>${esc(x.name)}</b></td><td>${esc(x.sku)}</td><td>${esc(x.category)}</td><td>${x.stock}</td><td>${x.demand}</td><td>${badge(x)}${x.hasOpenOrder ? ' <span class="badge b-ord">Ordered</span>' : ""}</td><td style="white-space:nowrap"><button class="btn ghost sm" data-edit="${x.id}">Edit</button> <button class="btn ghost sm" data-del="${x.id}" data-name="${esc(x.name)}">Delete</button></td></tr>`).join("")}</tbody></table>`;
}

async function restock() {
  const d = await api("/api/restock"), k = ui.rt, a = d.need, on = d.progress, dn = d.done;
  const T = [["need", "Needs Ordering", a.length], ["prog", "Orders in Progress", on.length], ["done", "Received / Completed", dn.length]];
  const th = h => `<thead><tr>${h.map(x => `<th>${x}</th>`).join("")}</tr></thead>`; let b;
  if (k === "need") b = a.length ? `<table>${th(["Product", "Status", "Available", "Suggested order", ""])}<tbody>${a.map(x => `<tr><td><b>${esc(x.name)}</b><br><small class="sub">${esc(x.why)}</small></td><td>${badge(x)}</td><td>${x.stock}</td><td><b>${x.suggestedQty}</b></td><td style="text-align:right"><button class="btn sm" data-ord="${x.id}" data-qty="${x.suggestedQty}" data-lead="${x.lead}" data-name="${esc(x.name)}" data-stock="${x.stock}">Place order</button></td></tr>`).join("")}</tbody></table>` : '<div class="empty">Nothing needs ordering right now.</div>';
  else if (k === "prog") b = on.length ? `<table>${th(["Product", "Quantity ordered", "Expected arrival", ""])}<tbody>${on.map(o => `<tr><td><b>${esc(o.name)}</b></td><td>${o.quantity}</td><td>${esc(o.expected || "Not set")}</td><td style="text-align:right;white-space:nowrap"><button class="btn sm" data-rec="${o.id}" data-qty="${o.quantity}" data-name="${esc(o.name)}">Confirm delivery</button> <button class="btn ghost sm" data-cancel="${o.id}">Cancel order</button></td></tr>`).join("")}</tbody></table>` : '<div class="empty">No orders in progress.</div>';
  else b = dn.length ? `<table>${th(["Product", "Quantity ordered", "Quantity received", "Result"])}<tbody>${dn.map(o => `<tr><td><b>${esc(o.name)}</b></td><td>${o.quantity}</td><td>${o.status === "Received" ? o.received : "—"}</td><td><span class="badge ${o.status === "Received" ? "b-ok" : "b-ord"}">${o.status}</span></td></tr>`).join("")}</tbody></table>` : '<div class="empty">No completed orders yet.</div>';
  return `<div class="head"><div><h1>Restock Actions</h1><p class="sub">Stock increases only when a delivery is received.</p></div></div>
  <div class="rtabs" role="tablist">${T.map(t => `<button role="tab" aria-selected="${k === t[0]}" data-rtab="${t[0]}" class="${k === t[0] ? "on" : ""}">${t[1]}<span>${t[2]}</span></button>`).join("")}</div>
  <div class="card tw pn" style="padding:6px 12px">${b}</div>`;
}

/* ---------- settings ---------- */
async function settings() {
  const t = ui.tab; let b;
  if (t === "appearance") { const cur = localStorage.getItem("stockora-theme") || "system"; b = `<h3>Theme</h3><div class="opt">${[["system", "Match device"], ["light", "Light"], ["dark", "Dark"]].map(([k, l]) => `<button data-set-theme="${k}" class="${cur === k ? "on" : ""}">${l}</button>`).join("")}</div>`; }
  else if (t === "rules") { const s = await api("/api/settings"); b = `<h3>Planning rules</h3><div class="fg" style="margin-top:12px"><label>Early warning (days)<input id="s-buf" type="number" min="0" step="1" value="${s.buffer}"><span class="hint">Flag low stock this many days early.</span></label><label>Order covers (days)<input id="s-rev" type="number" min="0" step="1" value="${s.review}"><span class="hint">How many days of sales a new order should cover.</span></label></div><div class="foot"><button class="btn" data-act="saveset">Save rules</button></div>`; }
  else b = `<h3>Data</h3><p class="sub">Export your products as a CSV file, or import products from one.</p><div class="foot" style="justify-content:flex-start"><button class="btn ghost" data-act="exp">Export CSV</button><button class="btn ghost" data-act="pick">Import CSV</button><input type="file" id="csvfile" accept=".csv,text/csv" hidden></div><hr style="border:0;border-top:1px solid var(--line);margin:20px 0"><div class="foot" style="justify-content:flex-start;margin:0"><button class="btn ghost" data-act="reset">Restore sample products</button><button class="btn danger" data-act="clear">Delete all data</button></div>`;
  return `<div class="head"><div><h1>Settings</h1></div></div><div class="set"><div class="stabs">${[["appearance", "Appearance"], ["rules", "Planning Rules"], ["data", "Data"]].map(([k, l]) => `<button data-stab="${k}" class="${t === k ? "on" : ""}">${l}</button>`).join("")}</div><div>${b}</div></div>`;
}
function applyTheme() { const t = localStorage.getItem("stockora-theme") || "system"; t === "system" ? document.documentElement.removeAttribute("data-theme") : document.documentElement.dataset.theme = t; }
function ask(title, text, label, attr) { openM(`<h2>${title}</h2><p class="sub">${text}</p><div class="foot"><button class="btn ghost" data-act="close">Cancel</button><button class="btn danger" ${attr}>${label}</button></div>`); }
async function act(a) {
  try {
    if (a === "close") closeM();
    else if (a === "add") prodForm();
    else if (a === "saveset") { await api("/api/settings", "PUT", { buffer: Number($("#s-buf").value), review: Number($("#s-rev").value) }); toast("Rules saved."); }
    else if (a === "exp") { const r = await fetch("/api/export"), u = URL.createObjectURL(await r.blob()), l = document.createElement("a"); l.href = u; l.download = "stockora-products.csv"; l.click(); URL.revokeObjectURL(u); toast("CSV exported."); }
    else if (a === "pick") $("#csvfile").click();
    else if (a === "reset") ask("Restore sample products?", "This replaces all current products and orders.", "Restore", 'data-dorst="1"');
    else if (a === "clear") ask("Delete all data?", "All products and orders will be removed. This can’t be undone.", "Delete everything", 'data-doclr="1"');
  } catch (e) { toast(e.message, true); }
}

/* ---------- forms ---------- */
const fld = (id, l, v, t = "text", x = "") => `<label>${l}<input id="f-${id}" type="${t}" value="${esc(v ?? "")}" ${x}><span class="err" id="e-${id}"></span></label>`;
const num = id => { const v = $("#f-" + id).value.trim(); return v === "" ? null : Number(v); };
function showErrors(e) { document.querySelectorAll(".err").forEach(x => x.textContent = ""); const er = e.data && e.data.errors; if (er) Object.keys(er).forEach(k => { const el = $("#e-" + k); if (el) el.textContent = er[k]; }); toast(e.message, true); }
async function prodForm(id) {
  const p = id ? await api("/api/products/" + id) : { category: "", stock: 0, demand: 0, lead: 7, safety: 0 };
  openM(`<h2>${id ? "Edit product" : "Add product"}</h2><div class="fg">${fld("sku", "Product ID", p.sku)}${fld("name", "Product name", p.name)}${fld("category", "Category", p.category)}${fld("stock", "Available quantity", p.stock, "number", 'min="0" step="1"')}${fld("demand", "Sold per day", p.demand, "number", 'min="0" step="any"')}${fld("lead", "Delivery time (days)", p.lead, "number", 'min="1" step="1"')}${fld("safety", "Minimum buffer stock", p.safety, "number", 'min="0" step="1"')}</div><div class="foot"><button class="btn ghost" data-act="close">Cancel</button><button class="btn" data-save="${id || ""}">${id ? "Save changes" : "Add product"}</button></div>`);
}
async function saveProd(id) {
  const body = { sku: $("#f-sku").value, name: $("#f-name").value, category: $("#f-category").value, stock: num("stock"), demand: num("demand"), lead: num("lead"), safety: num("safety") };
  try { await api(id ? "/api/products/" + id : "/api/products", id ? "PUT" : "POST", body); closeM(); await render(); toast(id ? "Changes saved." : "Product added."); } catch (e) { showErrors(e); }
}
function ordForm(b) {
  openM(`<h2>Place order: ${esc(b.name)}</h2><p class="sub">Available stock stays at ${b.stock} until the delivery is received.</p><div class="fg">${fld("oq", "Quantity to order", b.qty, "number", 'min="1" step="1"')}${fld("od", "Expected arrival", addDays(b.lead), "date")}</div><div class="foot"><button class="btn ghost" data-act="close">Cancel</button><button class="btn" data-doord="${b.id}">Place order</button></div>`);
}
async function doOrd(id) {
  try { await api("/api/orders", "POST", { productId: id, quantity: num("oq"), expected: $("#f-od").value }); ui.rt = "prog"; closeM(); await render(); toast("Order placed."); } catch (e) { showErrors(e); }
}
function recForm(b) {
  openM(`<h2>Confirm delivery: ${esc(b.name)}</h2><p class="sub">${b.qty} ordered.</p><div class="fg">${fld("rq", "Quantity received", b.qty, "number", 'min="1" step="1"')}</div><div class="foot"><button class="btn ghost" data-act="close">Cancel</button><button class="btn" data-dorec="${b.id}">Confirm delivery</button></div>`);
}
async function doRec(id) {
  try { await api(`/api/orders/${id}/receive`, "POST", { quantity: num("rq") }); ui.rt = "done"; closeM(); await render(); toast("Delivery recorded. Stock updated."); } catch (e) { showErrors(e); if (e.message.includes("already")) { closeM(); render(); } }
}
function delAsk(id, name) {
  openM(`<h2>Delete ${esc(name)}?</h2><p class="sub">This can’t be undone.</p><div class="foot"><button class="btn ghost" data-act="close">Cancel</button><button class="btn danger" data-dodel="${id}">Delete product</button></div>`);
}

/* ---------- routing ---------- */
const views = { overview, inventory, restock, settings };
function countUp(el) { const to = +el.textContent; if (!to) return; let s = null; const f = t => { s ??= t; const p = Math.min(1, (t - s) / 700); el.textContent = Math.round(to * (1 - Math.pow(1 - p, 3))); if (p < 1) requestAnimationFrame(f); }; requestAnimationFrame(f); }
async function render() {
  const h = location.hash.slice(1);
  if (h === "home") { document.body.dataset.view = "home"; return; }
  document.body.dataset.view = "app";
  const r = views[h] ? h : "overview";
  document.querySelectorAll(".side a[data-r]").forEach(a => a.classList.toggle("on", a.dataset.r === r));
  const m = $("#main");
  try { m.innerHTML = await views[r](); } catch (e) { m.className = "main"; m.innerHTML = location.protocol === "file:" ? '<div class="card empty"><b>This page was opened as a file.</b><br>Stockora needs its C++ server. Run <code>stockora</code> from this folder, then open <b>http://localhost:8080</b> in your browser.</div>' : '<div class="card empty">Can’t reach the server. Make sure the C++ program is running.</div>'; return; }
  m.className = "main" + (anim ? " enter" : "");
  if (anim && !matchMedia("(prefers-reduced-motion:reduce)").matches) document.querySelectorAll(".kpi b").forEach(countUp);
  anim = false;
}
addEventListener("hashchange", () => { anim = true; closeM(); render(); scrollTo(0, 0); });
addEventListener("keydown", e => { if (e.key === "Escape") closeM(); });

document.addEventListener("click", async e => {
  const t = e.target, g = a => t.closest("[" + a + "]"); let x;
  if (t.id === "modal") return closeM();
  if ((x = g("data-act"))) return act(x.dataset.act);
  if (g("data-dorst")) { try { await api("/api/reset", "POST"); closeM(); await render(); toast("Sample products restored."); } catch (er) { toast(er.message, true); } return; }
  if (g("data-doclr")) { try { await api("/api/data", "DELETE"); closeM(); await render(); toast("All data deleted."); } catch (er) { toast(er.message, true); } return; }
  if ((x = g("data-set-theme"))) { localStorage.setItem("stockora-theme", x.dataset.setTheme); applyTheme(); return render(); }
  if ((x = g("data-stab"))) { ui.tab = x.dataset.stab; return render(); }
  if ((x = g("data-kpi")) !== null) {
    const d = x.dataset; ui.status = ["ok", "low", "out", "lowout"].includes(d.kpi) ? d.kpi : "all"; ui.q = ""; ui.cat = "all";
    if (d.rt) ui.rt = d.rt; location.hash = d.to; return;
  }
  if ((x = g("data-rtab"))) { ui.rt = x.dataset.rtab; return render(); }
  if ((x = g("data-save"))) return saveProd(Number(x.dataset.save) || 0);
  if ((x = g("data-edit"))) return prodForm(Number(x.dataset.edit));
  if ((x = g("data-del"))) return delAsk(x.dataset.del, x.dataset.name);
  if ((x = g("data-dodel"))) { try { await api("/api/products/" + x.dataset.dodel, "DELETE"); closeM(); await render(); toast("Product deleted."); } catch (er) { closeM(); toast(er.message, true); } return; }
  if ((x = g("data-ord"))) return ordForm({ id: x.dataset.ord, qty: x.dataset.qty, lead: Number(x.dataset.lead), name: x.dataset.name, stock: x.dataset.stock });
  if ((x = g("data-doord"))) return doOrd(Number(x.dataset.doord));
  if ((x = g("data-rec"))) return recForm({ id: x.dataset.rec, qty: x.dataset.qty, name: x.dataset.name });
  if ((x = g("data-dorec"))) return doRec(Number(x.dataset.dorec));
  if ((x = g("data-cancel"))) { try { await api(`/api/orders/${x.dataset.cancel}/cancel`, "POST"); await render(); toast("Order cancelled."); } catch (er) { toast(er.message, true); } }
});
let timer;
document.addEventListener("input", e => { if (e.target.id === "q") { ui.q = e.target.value; clearTimeout(timer); timer = setTimeout(async () => { $("#tbl").innerHTML = await table(); }, 150); } });
document.addEventListener("change", async e => {
  if (e.target.id === "csvfile" && e.target.files[0]) { try { const d = await api("/api/import", "POST", { csv: await e.target.files[0].text() }); toast(`${d.added} imported${d.skipped ? `, ${d.skipped} skipped` : ""}.`, !d.added); } catch (er) { toast(er.message, true); } e.target.value = ""; return; } const m = { fs: "status", fc: "cat", so: "sort" }[e.target.id]; if (m) { ui[m] = e.target.value; $("#tbl").innerHTML = await table(); } });
applyTheme();
render();
