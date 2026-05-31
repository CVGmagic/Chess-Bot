extends GridContainer


@onready var chess_board: Control = %ChessBoard


func _ready():
	# Make sure the grid has exactly 8 columns
	add_theme_constant_override("h_separation", 0)
	add_theme_constant_override("v_separation", 0)
	


func generate_visual_board(num_columns=8, num_rows=8, white_square_color=Color("#f0d9b5"), black_square_color=Color("#b58863"), board_width=560, board_height=560):
	columns = num_columns
	for rank in range(num_rows):
		for file in range(num_columns):
			# Create a flat button or simple panel for each square
			var square = Button.new()
			
			# Force each square to be a uniform square size (e.g., 70x70 pixels)
			square.custom_minimum_size = Vector2(floor(board_width / num_columns), floor(board_width / num_rows))
			
			# Mathematical trick: alternating squares have even/odd sums of row + column
			var is_light_square = (rank + file) % 2 == 0
			
			# Create a simple flat look to easily apply custom colors
			var style = StyleBoxFlat.new()
			if is_light_square:
				style.bg_color = white_square_color # Classic light wood color
			else:
				style.bg_color = black_square_color # Classic dark wood color
				
			square.add_theme_stylebox_override("normal", style)
			square.add_theme_stylebox_override("hover", style)
			square.add_theme_stylebox_override("pressed", style)
			
			# Name the square node by its algebraic notation coordinates for debugging
			var file_letter = ["a","b","c","d","e","f","g","h"][file]
			var rank_number = str(num_rows - rank)
			square.name = file_letter + rank_number
			square.pressed.connect(func(): chess_board._on_square_clicked(rank, file))
			
			# Add the square to our GridContainer
			add_child(square)
