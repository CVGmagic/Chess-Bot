extends Node


var chess_engine: ChessEngine
@onready var chess_board: Control = %ChessBoard

var max_depth: int = 5

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	chess_engine = ChessEngine.new()
	set_engine_board(chess_board.board)
	
	for i in range(0, 0):
		print(chess_engine.perft(i))
	
	make_best_engine_move()


func make_random_engine_move() -> void:
	var raw_move: int = chess_engine.get_random_legal_move()
	if raw_move == 0:
		print("GAME OVER")
		
	var move: Array[int] = decode_move(raw_move)
	chess_board.make_move(move[0], move[1], move[2], move[3])
	print("Engine Move: (" + str(move[0]) + "," + str(move[1]) + ") (" + str(move[2]) + "," + str(move[3]) + ")")
	

func make_best_engine_move() -> void:
	# Instead of running it here and freezing the screen,
	# we throw the calculation onto a background thread task!
	WorkerThreadPool.add_task(Callable(self, "_async_engine_calculation"))


func _async_engine_calculation() -> void:
	# This runs entirely in the background. UI stays smooth!
	var raw_move: int = chess_engine.make_best_move(max_depth)
	
	# CRITICAL: You must never update UI elements from a background thread.
	# So, we use call_deferred to safely pass the result back to the main thread.
	call_deferred("_apply_engine_move", raw_move)


func _apply_engine_move(raw_move: int) -> void:
	# This runs back on the main thread safely
	if raw_move == 0:
		print("GAME OVER")
		return
		
	var move: Array[int] = decode_move(raw_move)
	chess_board.make_move(move[0], move[1], move[2], move[3], move[4])
	print("Engine Move: (" + str(move[0]) + "," + str(move[1]) + ") (" + str(move[2]) + "," + str(move[3]) + ")")
	
	
func set_engine_board(board: Array[Array]) -> void:
	var flat_array: PackedInt32Array = PackedInt32Array()
	flat_array.resize(64) # Allocate all 64 squares upfront for speed
	
	for rank in range(8):
		for file in range(8):
			flat_array[rank * 8 + file] = board[rank][file]
	
	chess_engine.set_board_from_array(flat_array, 0, 15) # 15 is 0b1111, so all castling rights intact


func try_move(from_rank: int, from_file: int, to_rank: int, to_file: int) -> int:
	var data: int = chess_engine.try_move(from_rank, from_file, to_rank, to_file, 4)
	# promo choice set to 4 (queen) by default
	return data
	
	
func decode_move(move: int) -> Array[int]:
	var from_sq_num: int = move & 63
	var to_sq_num: int = (move >> 6) & 63
	var flag: int = (move >> 12) & 15
	
	var from_rank: int = int(from_sq_num / 8)
	var from_file: int = from_sq_num % 8
	var to_rank: int = int(to_sq_num / 8)
	var to_file: int = to_sq_num % 8
	
	return [from_rank, from_file, to_rank, to_file, flag]
