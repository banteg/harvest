hook = {}
function hook.call(name, state)
    if name == "gameSave" then
        state.checkpoint = checkpoint
    elseif name == "gameLoad" then
        checkpoint = state.checkpoint
    end
end

-- Bootstrap scripts can return values; this leaves one on the stack before the save table.
return true
