import React, { useCallback, useEffect, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';
import './styles.css';

type Sound = { name: string; duration_ms: number };
type QueueItem = { id: number; audio_name: string; priority: number; enqueued_at: string };
type Deferred = { id: number; audio_name: string; priority: number; play_time: string };
type Entry = { audio_name: string; priority: number; time: string };
type Schedule = { name: string; enabled: boolean; entries: Entry[] };

const clientId = import.meta.env.VITE_GOOGLE_CLIENT_ID as string | undefined;
const tokenKey = 'racit-bell-google-id-token';

async function api<T>(path: string, token: string, options: RequestInit = {}): Promise<T> {
  const headers = new Headers(options.headers);
  headers.set('Authorization', `Bearer ${token}`);
  const response = await fetch(`/api${path}`, { ...options, headers });
  if (!response.ok) {
    const message = await response.text();
    throw new Error(response.status === 401 ? 'Your Google session is not authorized for this bell.' : message || `Request failed (${response.status})`);
  }
  if (response.status === 204 || !response.headers.get('content-length')) return undefined as T;
  const text = await response.text();
  return text ? JSON.parse(text) as T : undefined as T;
}

const duration = (ms: number) => `${Math.floor(ms / 60000)}:${String(Math.floor(ms / 1000) % 60).padStart(2, '0')}`;
const localTime = (iso: string) => new Date(iso).toLocaleString([], { dateStyle: 'medium', timeStyle: 'short' });

function GoogleLogin({ onToken }: { onToken: (value: string) => void }) {
  const button = useRef<HTMLDivElement>(null);
  const [error, setError] = useState('');
  useEffect(() => {
    if (!clientId) { setError('Google client ID is not configured. Set GOOGLE_CLIENT_ID before building the frontend.'); return; }
    const initialize = () => {
      const google = window.google;
      if (!google || !button.current) return;
      google.accounts.id.initialize({ client_id: clientId, callback: ({ credential }) => onToken(credential), auto_select: false });
      google.accounts.id.renderButton(button.current, { theme: 'outline', size: 'large', shape: 'pill', text: 'continue_with', width: 280 });
    };
    const existing = document.querySelector<HTMLScriptElement>('script[data-google-identity]');
    if (existing) { existing.addEventListener('load', initialize); initialize(); return () => existing.removeEventListener('load', initialize); }
    const script = document.createElement('script'); script.src = 'https://accounts.google.com/gsi/client'; script.async = true; script.dataset.googleIdentity = 'true'; script.onload = initialize; script.onerror = () => setError('Google Sign-In could not be loaded.'); document.head.append(script);
  }, [onToken]);
  return <main className="login"><section className="login-card"><div className="mark">R</div><p className="eyebrow">RACIT CONTROL</p><h1>Ring with confidence.</h1><p className="muted">Sign in with your authorized Google account to manage the bell system.</p><div ref={button} className="google-button" />{error && <p className="error">{error}</p>}</section></main>;
}

function App() {
  const [token, setToken] = useState(() => sessionStorage.getItem(tokenKey) || '');
  const signIn = useCallback((value: string) => { sessionStorage.setItem(tokenKey, value); setToken(value); }, []);
  const signOut = () => { sessionStorage.removeItem(tokenKey); setToken(''); };
  return token ? <Dashboard token={token} signOut={signOut} /> : <GoogleLogin onToken={signIn} />;
}

function Dashboard({ token, signOut }: { token: string; signOut: () => void }) {
  const [sounds, setSounds] = useState<Sound[]>([]); const [queue, setQueue] = useState<QueueItem[]>([]);
  const [deferred, setDeferred] = useState<Deferred[]>([]); const [schedules, setSchedules] = useState<Schedule[]>([]);
  const [volume, setVolume] = useState(1); const [notice, setNotice] = useState(''); const [error, setError] = useState('');
  const [active, setActive] = useState<'library' | 'queue' | 'deferred' | 'schedules'>('library');
  const [selected, setSelected] = useState(''); const [priority, setPriority] = useState(5); const [when, setWhen] = useState('');
  const [editing, setEditing] = useState<Schedule | null>(null);
  const fail = (e: unknown) => { const message = e instanceof Error ? e.message : 'Unexpected error'; setError(message); if (message.startsWith('Your Google session')) signOut(); };
  const load = useCallback(async () => { try { const [a, q, d, s, v] = await Promise.all([api<Sound[]>('/sounds', token), api<QueueItem[]>('/queue', token), api<Deferred[]>('/deferred', token), api<Schedule[]>('/schedule', token), api<{volume:number}>('/volume', token)]); setSounds(a); setQueue(q); setDeferred(d); setSchedules(s); setVolume(v.volume); setSelected(old => old || a[0]?.name || ''); } catch (e) { fail(e); } }, [token]);
  useEffect(() => { load(); }, [load]);
  const mutate = async (work: () => Promise<unknown>, message: string) => { setError(''); try { await work(); setNotice(message); await load(); } catch (e) { fail(e); } };
  const upload = (file?: File) => file && mutate(() => api('/sounds', token, { method: 'POST', body: (() => { const data = new FormData(); data.append('file', file); return data; })() }), `${file.name} uploaded.`);
  const setMasterVolume = (value: number) => { setVolume(value); void mutate(() => api('/volume', token, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ volume: value }) }), 'Volume updated.'); };
  return <div className="app"><header><div className="brand"><span className="mark small">R</span><span>RACIT Bell</span></div><nav>{(['library','queue','deferred','schedules'] as const).map(item => <button key={item} onClick={() => setActive(item)} className={active === item ? 'active' : ''}>{item === 'library' ? 'Sound library' : item}</button>)}</nav><button className="text-button" onClick={signOut}>Sign out</button></header>
    <main className="shell"><div className="page-title"><div><p className="eyebrow">BELL MANAGEMENT</p><h1>{active === 'library' ? 'Sound library' : active[0].toUpperCase() + active.slice(1)}</h1></div><button className="refresh" onClick={load}>↻ Refresh</button></div>{notice && <div className="notice" onClick={() => setNotice('')}>{notice}</div>}{error && <div className="error banner">{error}</div>}
    <section className="control-bar"><label>Master volume <input aria-label="Master volume" type="range" min="0" max="1" step="0.05" value={volume} onChange={e => setMasterVolume(+e.target.value)} /><b>{Math.round(volume * 100)}%</b></label><div className="quick"><select value={selected} onChange={e => setSelected(e.target.value)}><option value="">Select a sound</option>{sounds.map(s => <option key={s.name}>{s.name}</option>)}</select><input type="number" min="0" value={priority} onChange={e => setPriority(+e.target.value)} aria-label="Priority" /><button disabled={!selected} onClick={() => mutate(() => api('/queue', token, {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({audio_name:selected, priority})}), 'Playback requested.')}>Ring now</button><button className="secondary" onClick={() => mutate(() => api('/queue/skip', token, {method:'POST'}), 'Current playback skipped.')}>Skip current</button></div></section>
    {active === 'library' && <Library sounds={sounds} token={token} upload={upload} remove={name => mutate(() => api(`/sounds/${encodeURIComponent(name)}`, token, {method:'DELETE'}), 'Sound deleted.')} />}
    {active === 'queue' && <Queue queue={queue} remove={id => mutate(() => api(`/queue/${id}`, token, {method:'DELETE'}), 'Queue item removed.')} />}
    {active === 'deferred' && <DeferredView deferred={deferred} sounds={sounds} selected={selected} priority={priority} when={when} setWhen={setWhen} add={() => when && mutate(() => api('/deferred', token, {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({audio_name:selected,priority,play_time:new Date(when).toISOString().replace('.000','')})}), 'Playback scheduled.')} remove={id => mutate(() => api(`/deferred/${id}`, token, {method:'DELETE'}), 'Scheduled playback removed.')} />}
    {active === 'schedules' && <Schedules items={schedules} setEditing={setEditing} toggle={s => mutate(() => api(`/schedule/${encodeURIComponent(s.name)}/state`, token, {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({state:!s.enabled})}), 'Schedule updated.')} remove={name => mutate(() => api(`/schedule/${encodeURIComponent(name)}`, token, {method:'DELETE'}), 'Schedule deleted.')} />}
    </main>{editing !== null && <ScheduleEditor initial={editing} sounds={sounds} close={() => setEditing(null)} save={(schedule, update) => mutate(() => api('/schedule', token, {method:update?'PUT':'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(schedule)}), `Schedule ${update ? 'updated' : 'created'}.`).finally(() => setEditing(null))} />}</div>;
}

function Library({ sounds, upload, remove }: { sounds: Sound[]; token: string; upload: (f?: File) => void; remove: (n:string) => void }) { return <section className="panel"><div className="panel-head"><div><h2>Available sounds</h2><p className="muted">Upload a supported audio file, then use it immediately or in a schedule.</p></div><label className="button upload">Upload sound<input hidden type="file" accept="audio/*" onChange={e => upload(e.target.files?.[0])} /></label></div><div className="table">{sounds.length ? sounds.map(s => <div className="row" key={s.name}><span className="sound-icon">♪</span><b>{s.name}</b><span className="muted">{duration(s.duration_ms)}</span><button className="danger" onClick={() => remove(s.name)}>Delete</button></div>) : <Empty text="No sounds uploaded yet." />}</div></section>; }
function Queue({ queue, remove }: { queue: QueueItem[]; remove:(id:number)=>void }) { return <section className="panel"><div className="panel-head"><div><h2>Waiting queue</h2><p className="muted">Higher priority sounds play first.</p></div><span className="count">{queue.length} items</span></div><div className="table">{queue.length ? queue.map(item => <div className="row" key={item.id}><span className="number">{item.priority}</span><b>{item.audio_name}</b><span className="muted">Queued {localTime(item.enqueued_at)}</span><button className="danger" onClick={() => remove(item.id)}>Remove</button></div>) : <Empty text="Nothing is waiting to play." />}</div></section>; }
function DeferredView({ deferred, selected, priority, when, setWhen, add, remove }: { deferred: Deferred[]; sounds:Sound[]; selected:string; priority:number; when:string; setWhen:(v:string)=>void; add:()=>void; remove:(id:number)=>void }) { return <section className="panel"><div className="panel-head"><div><h2>Schedule a one-time ring</h2><p className="muted">Uses the sound and priority selected above.</p></div></div><div className="form-line"><input type="datetime-local" value={when} onChange={e => setWhen(e.target.value)} /><button disabled={!selected || !when} onClick={add}>Schedule playback</button></div><div className="table">{deferred.length ? deferred.map(item => <div className="row" key={item.id}><span className="number">{item.priority}</span><b>{item.audio_name}</b><span className="muted">{localTime(item.play_time)}</span><button className="danger" onClick={() => remove(item.id)}>Cancel</button></div>) : <Empty text="No one-time playbacks are scheduled." />}</div></section>; }
function Schedules({ items, setEditing, toggle, remove }: { items:Schedule[]; setEditing:(s:Schedule|null)=>void; toggle:(s:Schedule)=>void; remove:(name:string)=>void }) { return <section className="panel"><div className="panel-head"><div><h2>Daily schedules</h2><p className="muted">Each enabled schedule is evaluated every day.</p></div><button onClick={() => setEditing({name:'', enabled:true, entries:[{audio_name:'',priority:5,time:'08:00:00'}]})}>New schedule</button></div><div className="cards">{items.length ? items.map(s => <article className="schedule" key={s.name}><div><h3>{s.name}</h3><span className={s.enabled ? 'status on' : 'status'}>{s.enabled ? 'Enabled' : 'Disabled'}</span></div><p>{s.entries.map(e => `${e.time.slice(0,5)} · ${e.audio_name}`).join('  /  ')}</p><footer><button className="secondary" onClick={() => toggle(s)}>{s.enabled ? 'Disable' : 'Enable'}</button><button className="secondary" onClick={() => setEditing(s)}>Edit</button><button className="danger" onClick={() => remove(s.name)}>Delete</button></footer></article>) : <Empty text="No daily schedules created." />}</div></section>; }
function ScheduleEditor({ initial, sounds, close, save }: { initial:Schedule; sounds:Sound[]; close:()=>void; save:(s:Schedule, update:boolean)=>Promise<void> }) { const [schedule, setSchedule] = useState(initial); const updateEntry = (i:number, patch:Partial<Entry>) => setSchedule(s => ({...s, entries:s.entries.map((e, n) => n === i ? {...e,...patch} : e)})); const isUpdate = Boolean(initial.name); return <div className="modal-backdrop"><form className="modal" onSubmit={e => {e.preventDefault(); void save(schedule, isUpdate);}}><div className="panel-head"><h2>{isUpdate ? 'Edit schedule' : 'New schedule'}</h2><button type="button" className="text-button" onClick={close}>Close</button></div><label>Name<input required disabled={isUpdate} value={schedule.name} onChange={e => setSchedule({...schedule,name:e.target.value})} /></label><label className="check"><input type="checkbox" checked={schedule.enabled} onChange={e => setSchedule({...schedule,enabled:e.target.checked})} /> Enabled</label><h3>Rings</h3>{schedule.entries.map((entry, i) => <div className="entry" key={i}><input type="time" step="1" value={entry.time} onChange={e => updateEntry(i,{time:e.target.value})} /><select required value={entry.audio_name} onChange={e => updateEntry(i,{audio_name:e.target.value})}><option value="">Select sound</option>{sounds.map(s => <option key={s.name}>{s.name}</option>)}</select><input type="number" min="0" value={entry.priority} onChange={e => updateEntry(i,{priority:+e.target.value})} />{schedule.entries.length > 1 && <button type="button" className="danger" onClick={() => setSchedule({...schedule,entries:schedule.entries.filter((_,n)=>n!==i)})}>×</button>}</div>)}<button type="button" className="secondary" onClick={() => setSchedule({...schedule,entries:[...schedule.entries,{audio_name:'',priority:5,time:'08:00:00'}]})}>+ Add ring</button><div className="modal-actions"><button type="button" className="secondary" onClick={close}>Cancel</button><button type="submit">Save schedule</button></div></form></div>; }
function Empty({ text }: {text:string}) { return <div className="empty">{text}</div>; }

declare global { interface Window { google?: { accounts: { id: { initialize: (options: {client_id:string; callback:(r:{credential:string})=>void; auto_select:boolean})=>void; renderButton:(parent:HTMLElement, options:object)=>void } } } } }
createRoot(document.getElementById('root')!).render(<React.StrictMode><App /></React.StrictMode>);
