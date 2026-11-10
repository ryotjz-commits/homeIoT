// Novia Home Cloudflare Worker - command queue for hybrid remote access.
// Bind a KV namespace named NOVIA_KV. Set Worker secrets DEVICE_TOKEN and DASHBOARD_TOKEN.
// Deploy the dashboard static files separately with Cloudflare Pages; point app.js API_BASE to this Worker.
const cors = { "Access-Control-Allow-Origin": "*", "Access-Control-Allow-Methods": "GET,POST,OPTIONS", "Access-Control-Allow-Headers": "Content-Type,Authorization" };
const json = (data, status=200) => new Response(JSON.stringify(data), {status, headers:{...cors,"Content-Type":"application/json","Cache-Control":"no-store"}});
const authorized = (req, token) => req.headers.get("Authorization") === `Bearer ${token}`;
export default {
 async fetch(req, env) {
  const url = new URL(req.url), p=url.pathname;
  if (req.method==="OPTIONS") return new Response(null,{headers:cors});
  if (p==="/api/device/poll" && req.method==="GET") {
   if (!authorized(req,env.DEVICE_TOKEN)) return json({error:"unauthorized"},401);
   const command=await env.NOVIA_KV.get("command","json");
   if(command) await env.NOVIA_KV.delete("command");
   return json({command:command || null});
  }
  if (p==="/api/device/state" && req.method==="POST") {
   if (!authorized(req,env.DEVICE_TOKEN)) return json({error:"unauthorized"},401);
   const state=await req.json(); await env.NOVIA_KV.put("state",JSON.stringify(state),{expirationTtl:120});
   return json({ok:true});
  }
  if (p==="/api/state" && req.method==="GET") {
   if (!authorized(req,env.DASHBOARD_TOKEN)) return json({error:"unauthorized"},401);
   return json(await env.NOVIA_KV.get("state","json") || {relays:[false,false,false,false],wifi:false,blynk:false,ip:""});
  }
  if (p==="/api/relay" && req.method==="POST") {
   if (!authorized(req,env.DASHBOARD_TOKEN)) return json({error:"unauthorized"},401);
   const body=await req.json();
   if(!Number.isInteger(body.ch)||body.ch<0||body.ch>3||![0,1].includes(body.value)) return json({error:"invalid"},400);
   await env.NOVIA_KV.put("command",JSON.stringify({type:"relay",ch:body.ch,value:body.value,ts:Date.now()}),{expirationTtl:60});
   return json({queued:true});
  }
  if (p==="/api/all" && req.method==="POST") {
   if (!authorized(req,env.DASHBOARD_TOKEN)) return json({error:"unauthorized"},401);
   const body=await req.json(); if(![0,1].includes(body.value)) return json({error:"invalid"},400);
   await env.NOVIA_KV.put("command",JSON.stringify({type:"all",value:body.value,ts:Date.now()}),{expirationTtl:60});
   return json({queued:true});
  }
  return json({error:"not found"},404);
 }
};
