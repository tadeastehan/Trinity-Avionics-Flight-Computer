import os, json, segno, webbrowser, dash
from dash import html, dcc
import dash
import dash_leaflet as dl
import dash_daq as daq
from dotenv import load_dotenv
from dash.dependencies import Input, Output, State
import pandas as pd



preferences_file = "preferences.json"
preferences_json = {}

button_clicked = False

def load_env_file():
    load_dotenv()

def get_api_key():
    api_key = os.getenv('MAPY_CZ_API_KEY')
    return api_key

def load_preferences():
    if os.path.exists(preferences_file):
        with open(preferences_file, "r", encoding="utf-8") as file:
            return json.load(file)      
    return {"start_marker_position": [49.2388992, 16.5546350], "style": "aerial", "rocket_position": [49.2388992, 16.5547350]}

def save_preferences(preferences):
    with open(preferences_file, "w+", encoding="utf-8") as file:
        json.dump(preferences, file, indent=4, ensure_ascii=False)

def get_start_marker_position():
    return preferences_json["start_marker_position"]

def get_style():
    return preferences_json["style"]

def get_rocket_position():
    return preferences_json["rocket_position"]

load_env_file()
preferences_json = load_preferences()

logo = html.Img(
    className="item1",
    id="logo",
    src=r'assets/imgs/logo.png', alt='image'
)

# List CSV files in the data folder for dropdown
DATA_DIR = os.path.join(os.path.dirname(__file__), 'data')
file_options = [
    {"label": f, "value": f} for f in os.listdir(DATA_DIR) if f.endswith('.csv')
]

def get_column_options(file):
    if not file:
        return []
    csv_path = os.path.join(DATA_DIR, file)
    try:
        with open(csv_path, 'r', encoding='utf-8') as f:
            header = f.readline().strip().split(',')
        return [
            {"label": col, "value": col}
            for col in header if col.lower() not in ['time [ms]', 'time [delta_ms]']
        ]
    except Exception:
        return []

header_text = html.Div(
    className="item2",
    id="header_text",
    children=[
        html.P("Telemetry", id="header_text_p", className="header-title"),
        html.Div(
            id="header_text_controls",
            className="header-text-controls",
            children=[
                dcc.Dropdown(
                    id='file-dropdown',
                    options=[],  # Will be set by callback
                    value=None,  # Will be set by callback
                    clearable=False,
                    className="header-dropdown"
                ),
                html.Button(
                    id='refresh-files-button',
                    title='Refresh file list',
                    children=html.Img(src='assets/icons/database-refresh-outline.svg'),
                    n_clicks=0,
                    className="refresh-files-button"
                ),
                dcc.Dropdown(
                    id='data-column-dropdown',
                    options=[],  # Will be set by callback
                    value=None,  # Will be set by callback
                    clearable=False,
                    className="header-dropdown",
                    multi=True  # Allow multiple selection
                ),
                daq.ToggleSwitch(
                    id='streamed-data',
                    color='#fec036',
                    label='Stream',
                    value=False,
                    className="header-toggle-switch"
                ),
            ]
        )
    ]
)

rocket_map_pin = dict(
    iconUrl=r'assets/icons/rocket-launch.png',
    iconSize=[40, 40],
    iconAnchor=[20, 40],
)

start_map_pin = dict(
    iconUrl=r'assets/icons/helicopter.png',
    iconSize=[40, 40],
    iconAnchor=[20, 40],
)

map_div = html.Div(
    className="item3",
    children=[
        dl.Map(
            [
                dl.TileLayer(id="map-tiles",
                             url=f'https://api.mapy.cz/v1/maptiles/{{get_style()}}/256/{{z}}/{{x}}/{{y}}?apikey={{get_api_key()}}'),
                dl.Marker(id="start-marker", position=get_start_marker_position(), children=[dl.Tooltip("Launch")], icon=start_map_pin),
                dl.Marker(id="rocket-marker", position=get_rocket_position(), children=[dl.Tooltip("Rocket")], icon=rocket_map_pin),
                dl.Circle(center=get_rocket_position(), radius=5, color='rgba(50, 115, 197, 0.5)', id='rocket-radius')
            ],
            id='map',
            center=get_start_marker_position(),
            zoom=16,
        ),
        dcc.Dropdown(
            id='map-style-dropdown',
            options=[
                {'label': 'Aerial', 'value': 'aerial'},
                {'label': 'Basic', 'value': 'basic'},
                {'label': 'Tourist', 'value': 'outdoor'},
            ],
            value='aerial',
            clearable=False,
        ),
        html.Div(
            children=[
            html.Button(
                id='teleport-rocket',
                title='Go to Rocket',
                children=html.Img(src='assets/icons/rocket-outline.svg'),
                n_clicks=0,
            ),
            html.Button(
                id='teleport-home',
                title='Go to Start',
                children=html.Img(src='assets/icons/home-outline.svg'),
                n_clicks=0,
            ),
             html.Button(
                id='set-marker-button',
                title='Set Home Position',
                children=html.Img(src='assets/icons/home-edit-outline.svg'),
                n_clicks=0,
            ),
             html.Button(
                id='share-rocket-position',
                title='Share Rocket Position',
                children=html.Img(src='assets/icons/qrcode.svg'),
                n_clicks=0,
            ),
        ]),
        html.Div(id="placeholder")
    ]
)

altitude_graph = html.Div(
    className="item4",
    id = 'control-panel-altitude',
    children=[
        dcc.Graph(id='control-panel-altitude-graph', config={'displayModeBar': False})        
    ]
)

battery_indicator = html.Div(
    className="item6",
    id="control-panel-battery",
    children=[
        daq.Tank(
            id="control-panel-battery-component",
            label={"label": "Battery Voltage", "style": {"fontSize": 24}},
            units="V",
            min=6.0,
            max=8.5,
            value=7.8,  # Example value, adjust as needed
            showCurrentValue=True,
            color="#fec036",
        )
    ],
    n_clicks=0,
)

altitude_number = html.Div(
    id='control-panel-altitude-component-number',
    children=[
        daq.LEDDisplay(
            id='control-panel-current-number',
            value='000.00',
            label={'label':"Altitude", 'style':{'fontSize':24}},
            size=70,
            color='#fec036',
            style = {'color':'#black',},
            backgroundColor='#2b2b2b',
        )
    ],
    n_clicks=0,
)

max_altitude_number = html.Div(
    id='control-panel-max-altitude-component-number',
    children=[
        daq.LEDDisplay(
            id='control-panel-max-number',
            value='000.00',
            label={'label':"Maximum Altitude", 'style':{'fontSize':24}},
            size=70,
            color='#fec036',
            style={'color': '#black'},
            backgroundColor='#2b2b2b',
        )
    ],
    n_clicks=0,
)

stage_button = html.Div(
    className="stage_button_container",
    id="stage_button_component",
    children=[
        html.Button("INIT", className="stage_button", id="stage1"),
        html.Button("READY", className="stage_button", id="stage2"),
        html.Button("ARM", className="stage_button", id="stage3"),
        html.Button("ASCENT", className="stage_button", id="stage4"),
        html.Button("DEPLOY", className="stage_button", id="stage5"),
        html.Button("DESCENT", className="stage_button", id="stage6"),
        html.Button("LANDED", className="stage_button", id="stage7"),
    ],
)

console_log = html.Div(
    children=[
        html.Div( className="terminal", children=[html.Pre("", id='console-log')]),
    ]
)

app = dash.Dash(__name__,
    meta_tags=[
        {'name': 'viewport', 'content': 'width=device-width, initial-scale=1.0'}
    ],
    title='Space Force Telemetry',
    update_title='Space Force Telemetry')

app._favicon = r'assets/imgs/favicon.ico'

app.layout = html.Div(
    id='root',
    children=[
        html.Link(href='https://fonts.googleapis.com/css?family=Concert One',rel='stylesheet'),
        dcc.Interval(id='map-update-interval', interval=3000, n_intervals=0),  # Update map every 3 seconds
        dcc.Interval(id='graph-update-interval', interval=300, n_intervals=0),  # Update graph every 0.3 seconds
        dcc.Interval(id='console-log-interval', interval=1000, n_intervals=0),  # Update console log every second
        html.Div(
            className="grid-container",
            children=[
                logo,
                header_text,
                map_div,
                altitude_graph,
                battery_indicator,
                html.Div(className = "item5",id="altitude_item",children= [altitude_number]),
                html.Div(className="item9", id="data_column_item", children=[max_altitude_number]),
                html.Div(className="item8", children=[stage_button]),
                html.Div(className="item7", children=[console_log]),
            ]
        ),
    ]
)

@app.callback(
    Output('start-marker', 'n_clicks'),
    Input('set-marker-button', 'n_clicks')
)
def toggle_button_clicked(n_clicks):
    global button_clicked  # Access the global variable
    if n_clicks:
        button_clicked = True
    return None  # Reset button state after updating marker position

@app.callback(
    Output('start-marker', 'position'),
    Input('map', 'clickData')
)
def update_marker_position(clickData):
    global button_clicked
    if button_clicked:
        button_clicked = False
        if clickData is not None:
            lat = clickData['latlng']['lat']
            lon = clickData['latlng']['lng']
            preferences_json["start_marker_position"] = [lat, lon]
            save_preferences(preferences_json)
            return [lat, lon]
    return dash.no_update

@app.callback(
    Output('map-tiles', 'url'),
    Input('map-style-dropdown', 'value')
)
def update_map_style(value):
    if value is not None:
        preferences_json["style"] = value
        save_preferences(preferences_json)
    return f'https://api.mapy.cz/v1/maptiles/{value}/256/{{z}}/{{x}}/{{y}}?apikey={get_api_key()}'

@app.callback(
    Output("placeholder", "children"),
    [Input("share-rocket-position", "n_clicks")], prevent_initial_call=True
)
def open_qr_code_tab(n_clicks):
    if n_clicks:
        lat, lon = get_rocket_position()

        url = f"https://mapy.cz/letecka?q={lat},{lon}&z=20"
        qr_code = segno.make_qr(url)
        qr_code.save("assets/qr_code.png", scale=10)

        webbrowser.open_new_tab("http://127.0.0.1:8050/assets/qr_code.png")
    return None

# Remove static file_options, and add a function to scan files

def scan_file_options():
    return [
        {"label": f, "value": f} for f in os.listdir(DATA_DIR) if f.endswith('.csv')
    ]

# Callback to update file-dropdown options and value
@app.callback(
    [Output('file-dropdown', 'options'), Output('file-dropdown', 'value')],
    [Input('refresh-files-button', 'n_clicks')],
    prevent_initial_call=False
)
def update_file_options(n_clicks):
    options = scan_file_options()
    value = options[0]['value'] if options else None
    return options, value

# Update data-column-dropdown callback to use State for file-dropdown options
@app.callback(
    [Output('data-column-dropdown', 'options'), Output('data-column-dropdown', 'value')],
    [Input('file-dropdown', 'value')],
    [State('file-dropdown', 'options')]
)
def update_column_options(selected_file, file_options):
    options = get_column_options(selected_file)
    value = options[0]['value'] if options else None
    return options, value

# Combined callback for file, column, streamed toggle, and interval
@app.callback(
    [
        Output('control-panel-altitude-graph', 'figure'),
        Output('control-panel-current-number', 'value'),
        Output('control-panel-max-number', 'value'),
        Output('control-panel-current-number', 'label'),
        Output('control-panel-max-number', 'label'),
        Output('control-panel-battery-component', 'value'),
        *[Output(f'stage{i+1}', 'style') for i in range(7)],
    ],
    [
        Input('file-dropdown', 'value'),
        Input('data-column-dropdown', 'value'),
        Input('streamed-data', 'value'),
        Input('graph-update-interval', 'n_intervals')
    ],
    prevent_initial_call=False
)
def update_graph_outputs(selected_file, selected_columns, streamed, n_intervals):
    ctx = dash.callback_context
    # Streaming logic
    if streamed:
        if ctx.triggered and any('graph-update-interval' in t['prop_id'] for t in ctx.triggered):
            pass
        else:
            raise dash.exceptions.PreventUpdate
    else:
        if ctx.triggered and any(('file-dropdown' in t['prop_id'] or 'data-column-dropdown' in t['prop_id']) for t in ctx.triggered):
            pass
        else:
            raise dash.exceptions.PreventUpdate
    # Defaults
    fig = {}
    current_val_string = '000.00'
    max_val_string = '000.00'
    label = {'label': 'Value', 'style': {'fontSize': 24}}
    max_label = {'label': 'Maximum Value', 'style': {'fontSize': 24}}
    battery_val = 0
    stages = ["blanchedalmond"] * 7
    stage_colors = [
        "rgb(255, 153, 0)", "rgb(255, 200, 0)", "rgb(255, 224, 0)",
        "rgb(255, 247, 0)", "rgb(184, 245, 0)", "rgb(149, 226, 20)", "rgb(114, 206, 39)"
    ]
    try:
        if not selected_file or not selected_columns:
            return fig, current_val_string, max_val_string, label, max_label, battery_val, *tuple({"background-color": color} for color in stages)
        csv_path = os.path.join(DATA_DIR, selected_file)
        df = pd.read_csv(csv_path)
        # Battery voltage
        if 'Battery[V]' in df.columns:
            try:
                battery_val = float(df['Battery[V]'].iloc[-1])
            except Exception:
                battery_val = 0
        else:
            battery_val = 0
        # Graph
        x_col = None
        for col in df.columns:
            if col.lower() in ['time [ms]', 'time [delta_ms]', 'timestamp', 'time']:
                x_col = col
                break
        if not x_col:
            x_col = df.columns[0]
        graph_data = []
        # Support both single and multiple selection
        if not isinstance(selected_columns, list):
            selected_columns = [selected_columns] if selected_columns else []
        # Plot each selected column on a separate y-axis
        yaxis_ids = ['y', 'y2', 'y3', 'y4', 'y5']
        yaxis_layout = {
            'yaxis': {'title': selected_columns[0] if selected_columns else '', 'side': 'left', 'titlefont': {'color': '#fec036'}, 'tickfont': {'color': '#fec036'}},
        }
        for i, col in enumerate(selected_columns):
            axis_id = yaxis_ids[i] if i < len(yaxis_ids) else f'y{i+1}'
            axis_ref = '' if i == 0 else axis_id
            graph_data.append({
                'x': df[x_col] / 1000 if 'ms' in x_col.lower() else df[x_col],
                'y': df[col],
                'type': 'line',
                'name': col,
                'yaxis': axis_ref
            })
            if i > 0:
                yaxis_layout[f'yaxis{i+1}'] = {
                    'title': col,
                    'side': 'right' if i % 2 == 1 else 'left',
                    'overlaying': 'y',
                    'titlefont': {'color': '#fec036'},
                    'tickfont': {'color': '#fec036'}
                }
        # State change markers only for the first selected column
        if selected_columns and 'State[x]' in df.columns:
            state_col = 'State[x]'
            state_changes = df[state_col].ne(df[state_col].shift()).fillna(False)
            change_indices = df.index[state_changes].tolist()
            state_labels = {
                1: 'Init', 2: 'Ready', 3: 'Arm', 4: 'Ascent', 5: 'Deploy', 6: 'Descent', 7: 'Landed',
            }
            marker_x = (df.loc[change_indices, x_col] / 1000 if 'ms' in x_col.lower() else df.loc[change_indices, x_col])
            marker_y = df.loc[change_indices, selected_columns[0]]
            marker_text = [f"{state_labels.get(int(s), int(s))}" for s in df.loc[change_indices, state_col]]
            graph_data.append({
                'x': marker_x,
                'y': marker_y,
                'mode': 'markers+text',
                'type': 'scatter',
                'marker': {'size': 12, 'color': '#fec036', 'symbol': 'diamond'},
                'text': marker_text,
                'textposition': 'top center',
                'name': '',
                'showlegend': False,
                'yaxis': ''
            })
        # Current/max values for the first column
        if selected_columns and selected_columns[0] in df.columns:
            current_val = df[selected_columns[0]].iloc[-1]
            max_val = df[selected_columns[0]].max()
            label_text = selected_columns[0].replace('_', ' ')
            current_val_string = f"{current_val:.2f}"
            max_val_string = f"{max_val:.2f}"
            label = {'label': label_text, 'style': {'fontSize': 24}}
            max_label = {'label': f"Maximum {label_text}", 'style': {'fontSize': 24}}
            if 'pressure' in selected_columns[0].lower():
                current_val_string = f"{current_val:.0f}"
                max_val_string = f"{max_val:.0f}"
            elif 'altitude' in selected_columns[0].lower():
                current_val_string = f"{current_val:.0f}"
                max_val_string = f"{max_val:.0f}"
        fig = {
            'data': graph_data,
            'layout': {
                'title': ', '.join(selected_columns),
                'xaxis': {'title': x_col.replace('ms', 's').replace('MS', 's').replace('Ms', 's') if 'ms' in x_col.lower() else x_col},
                'plot_bgcolor': '#2b2b2b',
                'paper_bgcolor': '#2b2b2b',
                'font': {'color': '#fec036'},
                'showlegend': False,
                **yaxis_layout
            }
        }
        # Stage colors
        if 'State[x]' in df.columns:
            try:
                stage_number = int(df['State[x]'].iloc[-1])
                if 1 <= stage_number <= 7:
                    stages[:stage_number] = stage_colors[:stage_number]
            except Exception:
                pass
    except Exception:
        pass
    return fig, current_val_string, max_val_string, label, max_label, battery_val, *tuple({"background-color": color} for color in stages)

# Callback for map
@app.callback(
    Output('map', 'children'),
    [
        Input('file-dropdown', 'value'),
        Input('data-column-dropdown', 'value'),
        Input('streamed-data', 'value'),
        Input('map-update-interval', 'n_intervals')
    ],
    prevent_initial_call=False
)
def update_map_outputs(selected_file, selected_column, streamed, n_intervals):
    ctx = dash.callback_context
    # Streaming logic
    if streamed:
        if ctx.triggered and any('map-update-interval' in t['prop_id'] for t in ctx.triggered):
            pass
        else:
            raise dash.exceptions.PreventUpdate
    else:
        if ctx.triggered and any(('file-dropdown' in t['prop_id'] or 'data-column-dropdown' in t['prop_id']) for t in ctx.triggered):
            pass
        else:
            raise dash.exceptions.PreventUpdate
    # Map logic
    lat_col = None
    lon_col = None
    time_col = None
    map_children = []
    rocket_marker_position = get_rocket_position()
    try:
        if not selected_file or not selected_column:
            return map_children
        csv_path = os.path.join(DATA_DIR, selected_file)
        df = pd.read_csv(csv_path)
        for col in df.columns:
            if 'latitude' in col.lower():
                lat_col = col
            if 'longitude' in col.lower():
                lon_col = col
            if col.lower() in ['time [ms]', 'time [delta_ms]', 'timestamp', 'time']:
                time_col = col
        points = []
        if lat_col and lon_col and time_col:
            valid = df[(df[lat_col] != 0) & (df[lon_col] != 0)].copy()
            freq_df = valid.groupby([lat_col, lon_col], as_index=False).size()
            freq_df.rename(columns={'size': 'freq'}, inplace=True)
            latest_time = valid.groupby([lat_col, lon_col], as_index=False)[time_col].max()
            merged = pd.merge(freq_df, latest_time, on=[lat_col, lon_col])
            freq_min = merged['freq'].min()
            freq_max = merged['freq'].max()
            t_min = merged[time_col].min()
            last_lat = valid[lat_col].iloc[-1]
            last_lon = valid[lon_col].iloc[-1]
            mask = (valid[lat_col] == last_lat) & (valid[lon_col] == last_lon)
            idx_first = mask.idxmax() if mask.any() else valid.index[-1]
            t_max = valid.loc[idx_first, time_col]
            def time_to_color(t):
                colors = [
                    (255, 0, 0), (255, 167, 0), (255, 244, 0), (163, 255, 0), (44, 186, 0)
                ]
                if t_max == t_min:
                    return 'rgb(255,0,0)'
                ratio = (t - t_min) / (t_max - t_min)
                ratio = max(0, min(ratio, 1))
                n = len(colors) - 1
                seg = min(int(ratio * n), n - 1)
                seg_start = seg / n
                seg_end = (seg + 1) / n
                local_ratio = (ratio - seg_start) / (seg_end - seg_start)
                r1, g1, b1 = colors[seg]
                r2, g2, b2 = colors[seg + 1]
                r = int(r1 + (r2 - r1) * local_ratio)
                g = int(g1 + (g2 - g1) * local_ratio)
                b = int(b1 + (b2 - b1) * local_ratio)
                return f'rgb({r},{g},{b})'
            if not valid.empty:
                last_valid_lat = valid[lat_col].iloc[-1]
                last_valid_lon = valid[lon_col].iloc[-1]
                rocket_marker_position = [last_valid_lat, last_valid_lon]
            else:
                rocket_marker_position = get_rocket_position()
            points = [
                dl.CircleMarker(
                    center=[row[lat_col], row[lon_col]],
                    radius=5 + 10 * ((row['freq'] - freq_min) / (freq_max - freq_min)) if freq_max > freq_min else 10,
                    color=time_to_color(row[time_col]),
                    fill=True,
                    fillOpacity=0.8,
                    fillColor=time_to_color(row[time_col])
                )
                for _, row in merged.sort_values(time_col).iterrows()
            ]
            preferences_json["rocket_position"] = rocket_marker_position
            map_children = [
                dl.TileLayer(id="map-tiles",
                             url=f'https://api.mapy.cz/v1/maptiles/{{get_style()}}/256/{{z}}/{{x}}/{{y}}?apikey={{get_api_key()}}'),
                dl.Marker(id="start-marker", position=get_start_marker_position(), children=[dl.Tooltip("Launch")], icon=start_map_pin),
                dl.Marker(id="rocket-marker", position=rocket_marker_position, children=[dl.Tooltip("Rocket")], icon=rocket_map_pin),
                dl.Circle(center=rocket_marker_position, radius=5, color='rgba(50, 115, 197, 0.5)', id='rocket-radius'),
            ] + points
        elif lat_col and lon_col:
            points = [
                dl.Marker(position=[row[lat_col], row[lon_col]])
                for _, row in df.iterrows()
                if row[lat_col] != 0 and row[lon_col] != 0
            ]
            map_children = [
                dl.TileLayer(id="map-tiles",
                             url=f'https://api.mapy.cz/v1/maptiles/{{get_style()}}/256/{{z}}/{{x}}/{{y}}?apikey={{get_api_key()}}'),
                dl.Marker(id="start-marker", position=get_start_marker_position(), children=[dl.Tooltip("Launch")], icon=start_map_pin),
                dl.Marker(id="rocket-marker", position=rocket_marker_position, children=[dl.Tooltip("Rocket")], icon=rocket_map_pin),
                dl.Circle(center=rocket_marker_position, radius=5, color='rgba(252, 186, 3, 0.5)', id='rocket-radius'),
            ] + points
        else:
            map_children = [
                dl.TileLayer(id="map-tiles",
                             url=f'https://api.mapy.cz/v1/maptiles/{{get_style()}}/256/{{z}}/{{x}}/{{y}}?apikey={{get_api_key()}}'),
                dl.Marker(id="start-marker", position=get_start_marker_position(), children=[dl.Tooltip("Launch")], icon=start_map_pin),
                dl.Marker(id="rocket-marker", position=rocket_marker_position, children=[dl.Tooltip("Rocket")], icon=rocket_map_pin),
                dl.Circle(center=rocket_marker_position, radius=5, color='rgba(252, 186, 3, 0.5)', id='rocket-radius'),
            ]
    except Exception:
        pass
    return map_children

@app.callback(
    Output('map', 'center'),
    [Input('teleport-rocket', 'n_clicks'), Input('teleport-home', 'n_clicks')],
    [State('map', 'center')]
)
def teleport_map(n_rocket, n_home, current_center):
    ctx = dash.callback_context
    if not ctx.triggered:
        return current_center
    button_id = ctx.triggered[0]['prop_id'].split('.')[0]
    if button_id == 'teleport-rocket':
        # Get rocket position and update preferences
        pos = get_rocket_position()
        return pos
    elif button_id == 'teleport-home':
        # Get start-marker position and update preferences
        pos = get_start_marker_position()
        return pos
    return current_center

# Special callback to print timestamp and latest row in formatted style, updating every 1 second
@app.callback(
    Output('console-log', 'children'),
    [Input('file-dropdown', 'value'), Input('console-log-interval', 'n_intervals')]
)
def update_console_log_fancy(selected_file, n_intervals):
    import time
    if not selected_file:
        return ''
    csv_path = os.path.join(DATA_DIR, selected_file)
    try:
        with open(csv_path, 'r', encoding='utf-8') as f:
            header_line = f.readline().strip()
        headers = [h.strip() for h in header_line.split(',')]
        df = pd.read_csv(csv_path)
        if not df.empty:
            latest_row = df.iloc[-1]
            timestamp = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
            # Build two columns: left (headers), right (values)
            left_col = []
            right_col = []
            for h, v in zip(headers, latest_row.values):
                left_col.append(html.Div(h, className='console-log-header'))
                right_col.append(html.Div(str(v), className='console-log-value'))
            return html.Div([
                html.Div(timestamp, className='console-log-timestamp'),
                html.Div([
                    html.Div(left_col, className='console-log-left'),
                    html.Div(right_col, className='console-log-right')
                ], className='console-log-row')
            ])
        else:
            return f"No data rows."
    except Exception as e:
        return f"Error reading file: {e}"

if __name__ == '__main__': 
    app.run(debug=True, port=8050)
