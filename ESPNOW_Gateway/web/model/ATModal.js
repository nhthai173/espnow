class ATModal {
    static currentModal = null

    static init() {
        document.body.addEventListener('click', (e) => {
            const el = e.target
            const action = el.getAttribute('action')
            switch (action) {
                case 'atDiscard':
                    Confirm.show('Discard', 'Are you sure you want to discard changes?', 'Discard', 'Cancel', () => {
                        ATModal.currentModal?.discard()
                    })
                    break
                case 'atSave':
                    ATModal.currentModal?.save()
                    break
                case 'atDelete':
                    Confirm.show('Delete', 'Are you sure you want to delete this?', 'Delete', 'Cancel', () => {
                        ATModal.currentModal?.delete()
                    })
                    break
            }
        })

        try {
            ATEdit.init()
            ATEditItem.init()
        } catch (e) { }

    }
}




/**
 * Modal Add/edit automation
 */
class ATEdit extends ATModal {

    static init() {
        // match type select
        let matchTypes = []
        document.querySelectorAll("#atEditModal [name='atMatchType'] .select-option")?.forEach(el => {
            const opt = document.createElement('li')
            opt.className = 'dropdown-item pointer'
            opt.innerHTML = el.innerHTML
            opt.setAttribute('value', el.getAttribute('value'))
            matchTypes.push(opt)
        })
        if (matchTypes.length) {
            const el = document.querySelector("#atEditModal [name='atMatchType']")
            el.setAttribute('data-bs-toggle', 'dropdown')
            const menu = document.createElement('div')
            menu.className = 'dropdown-menu'
            matchTypes.forEach(opt => {
                menu.appendChild(opt)
                opt.addEventListener('click', () => {
                    el.querySelectorAll('.select-option').forEach(opt => opt.removeAttribute('selected'))
                    el.querySelector(`[value="${opt.getAttribute('value')}"]`).setAttribute('selected', '')
                })
            })
            el.parentNode.appendChild(menu)
        }

        // edit item
        document.getElementById("atEditModal").addEventListener('click', (e) => {
            const el = e.target
            const item = el.closest('.at-item')
            if (!item) return
            const type = item.closest('.card')?.getAttribute('at-type')
            const action = item.classList.contains('at-new') ? 'add' : (item.classList.contains('at-edit') ? 'edit' : null)
            if (!type || !action) return
            const data = {
                previousModal: ATModal.currentModal,
                type,
                index: item.getAttribute('index') || '',
                dname: item.getAttribute('dname') || '',
                did: item.getAttribute('did') || '',
                prop: item.getAttribute('prop') || '',
                value: item.getAttribute('value') || ''
            }
            setTimeout(() => {
                new ATEditItem(type, action, data)
            }, 100);
            if (!isMobile) {
                ATModal.currentModal?.destroy()
            }
        })

    }

    constructor(action, data, atObject = null) {
        super()
        ATModal.currentModal = this

        this.modal = document.getElementById('atEditModal')
        this.action = action
        this.data = data || {}
        this.atObject = atObject
        this.$title = this.modal.querySelector('[role="title"]')
        this.$delBtn = this.modal.querySelector('[action="atDelete"]')
        this.$name = this.modal.querySelector('[name="atName"]')
        this.$matchType = this.modal.querySelector('[name="atMatchType"]')
        this.$triggers = this.modal.querySelector('.card[at-type="trigger"] .card-body')
        this.$actions = this.modal.querySelector('.card[at-type="action"] .card-body')

        if (action === 'add') {
            this.data = {
                id: '',
                enabled: true,
                name: '',
                matchType: '0',
                triggers: [],
                actions: []
            }
            this.$delBtn.style.display = 'none'
            this.$title.textContent = 'Add new automation'
        } else {
            this.oldData = duplicateObject(data)
            this.$delBtn.style.display = 'block'
            this.$title.textContent = 'Edit automation'
        }

        this._load()
        this.$name.value = data.name || ''
        if (!isNaN(Number(data.matchType))) {
            this.$matchType.querySelectorAll('.select-option').forEach(opt => opt.removeAttribute('selected'))
            this.$matchType.querySelector(`[value="${data.matchType}"]`)?.setAttribute('selected', '')
        }

        new bootstrap.Modal(this.modal).show()
    }

    // clear all items from HTML then reload data
    _load() {
        this.$triggers.querySelectorAll('.at-item.at-edit').forEach(item => item.remove())
        this.$actions.querySelectorAll('.at-item.at-edit').forEach(item => item.remove())
        this.data.triggers.forEach((trigger, index) => {
            trigger.index = index
            this._appendHTML('trigger', trigger)
        })
        this.data.actions.forEach((action, index) => {
            action.index = index
            this._appendHTML('action', action)
        })
    }

    // get item by index from HTML
    _get(type, index) {
        let el = this.modal.querySelector(`.card[at-type="${type}"] .at-item.at-edit[index="${index}"]`)
        if (el) {
            return {
                el,
                index: index,
                did: el.getAttribute('did'),
                dname: el.getAttribute('dname'),
                atType: el.getAttribute('type'),
                prop: el.getAttribute('prop'),
                value: el.getAttribute('value')
            }
        }
        return null
    }

    // set item by index to HTML, if not exists, add new
    _set(type, data) {
        let el = this._get(type, data.index)
        if (!el) {
            this._add(type, data)
        } else {
            this.data[`${type}s`][data.index] = data
            el.el.querySelector('.title').innerHTML = data.value
            el.el.querySelector('.subtitle').innerHTML = `${data.dname} | ${data.prop}`
            el.el.setAttribute('did', data.did)
            el.el.setAttribute('type', data.atType)
            el.el.setAttribute('dname', data.dname)
            el.el.setAttribute('prop', data.prop)
            el.el.setAttribute('value', data.value)
        }
    }

    // add new item to HTML, if exists, show error
    _add(type, data) {
        let index = this.data[`${type}s`].findIndex(item => item.did == data.did && item.prop == data.prop && item.value == data.value)
        if (index >= 0) {
            Confirm.show('Duplicate', 'This item already exists', 'OK')
            return
        }
        this.data[`${type}s`].push(data)
        data.index = this.data[`${type}s`].length - 1
        this._appendHTML(type, data)
    }

    // append item to HTML
    _appendHTML(type, data) {
        let index = Number(data.index)
        if (isNaN(index)) return
        let el = document.createElement('div')
        el.className = 'at-item at-edit'
        el.setAttribute('index', index)
        el.setAttribute('did', data.did)
        el.setAttribute('type', data.atType)
        el.setAttribute('dname', data.dname)
        el.setAttribute('prop', data.prop)
        el.setAttribute('value', data.value)
        el.innerHTML = `
                    <div class="index">${index + 1}</div>
                    <div class="content">
                        <div class="title">${data.value}</div>
                        <div class="subtitle">${data.dname} | ${data.prop}</div>
                    </div>`
        this.modal.querySelector(`.card[at-type="${type}"] .card-body`).insertBefore(el, this.modal.querySelector(`.card[at-type="${type}"] .card-body .at-new`))
    }

    // remove item from HTML then reload data
    _remove(type, index) {
        this.modal.querySelector(`.card[at-type="${type}"] .at-item.at-edit[index="${index}"]`)?.remove()
        if (this.data[`${type}s`].length) {
            this.data[`${type}s`].splice(index, 1)
        }
        this._load()
    }

    destroy() {
        ATModal.currentModal = null
        bootstrap.Modal.getInstance(this.modal)?.hide()
        document.querySelectorAll('.modal-backdrop').forEach(m => m.remove())
        this.atObject?.handleChanges(this.data)
    }

    save() {
        const { $name, $triggers, $actions, $matchType, data } = this
        const name = $name.value
        const match = $matchType.querySelector('.select-option[selected]')?.getAttribute('value')
        const triggers = data.triggers
        const actions = data.actions
        if (name == '' || isNaN(match) || triggers.length == 0 || actions.length == 0) {
            Confirm.show('Please fill all fields', 'Name, match type, triggers and actions are required', 'OK')
            return
        }
        data.name = name
        data.matchType = match
        // this.sendSaveRequest(data)
        data.lastAction = this.action === 'add' ? 'add' : 'save'
        console.log(data)
        this.destroy()
    }

    discard() {
        if (this.action === 'edit') {
            this.data = this.oldData
        }
        this.data.lastAction = 'discard'
        this.destroy()
    }

    delete() {
        // this.sendDeleteRequest(this.data)
        this.data.lastAction = 'delete'
        this.destroy()
    }

    handleItemAction(data) {
        console.log(data)

        if (data.lastAction == 'delete') {
            this._remove(data.type, data.index)
        } else if (data.lastAction == 'save') {
            this._set(data.type, data)
        }

        new bootstrap.Modal('#atEditModal').show()
        ATModal.currentModal = data.previousModal
    }
}





/**
 * Modal add/edit trigger/action
 */ 
class ATEditItem extends ATModal {

    static init() {
        document.querySelectorAll("#atEditItemModal [name]").forEach(el => {
            el.addEventListener('change', () => {
                const name = el.getAttribute('name')
                if (name === 'did') {
                    ATModal.currentModal?._selectDevice(el.value)
                    ATModal.currentModal?._intPropsSelect()
                }
            })
        })
    }

    constructor(type, action, data) {
        super()
        ATModal.currentModal = this

        this.modal = document.getElementById('atEditItemModal')
        this.data = data
        this.action = action
        this.$devices = this.modal.querySelector('[name="did"]')
        this.$props = this.modal.querySelector('[name="prop"]')
        this.$value = this.modal.querySelector('[name="value"]')
        this.$delBtn = this.modal.querySelector('[action="atDelete"]')

        this.titleText = action.charAt(0).toUpperCase() + action.slice(1)
        this.titleText += ` ${type}`
        this.deleteText = `Delete ${type}`
        this.modal.querySelector('[role="title"]').innerHTML = this.titleText
        this.$delBtn.innerHTML = this.deleteText

        this._selectDevice(this.data.did)
        this.$devices.dispatchEvent(new Event('change'))
        this._intPropsSelect()
        this._propSelect(this.data.prop)
        this.$value.value = this.data.value || ""

        if (action === 'edit') {
            this.$devices.disabled = true
            this.$delBtn.style.display = 'block'
        } else {
            this.$devices.disabled = false
            this.$delBtn.style.display = 'none'
        }

        // @TODO support all type
        this.data.atType = '2'

        new bootstrap.Modal(this.modal).show()
    }

    destroy() {
        ATModal.currentModal = null
        bootstrap.Modal.getInstance(this.modal)?.hide()
        this.data.previousModal?.handleItemAction(this.data)
    }

    _initDevicesSelect() {
        const { $devices } = this
        $devices.innerHTML = ''
        for (let device of Devices) {
            let option = document.createElement('option')
            option.value = device.did
            option.setAttribute('dname', device.name)
            option.innerHTML = `${device.name} (${device.did})`
            $devices.appendChild(option)
        }
    }
    _selectDevice(did) {
        const { $devices } = this
        if (!did || did == 'null') {
            this._initDevicesSelect()
            // null option
            let nullOpt = document.createElement('option')
            nullOpt.value = 'null'
            nullOpt.innerHTML = 'Select a device'
            nullOpt.disabled = true
            nullOpt.selected = true
            $devices.setAttribute('dname', '')
            $devices.prepend(nullOpt)
            return
        }
        if (this.action === 'edit') {
            $devices.innerHTML = ''
            let device = Devices.find(device => device.did === did)
            if (!device) return
            let option = document.createElement('option')
            option.value = device.did
            option.innerHTML = `${device.name} (${device.did})`
            option.disabled = true
            option.selected = true
            $devices.appendChild(option)
            $devices.setAttribute('dname', device.name)
        } else {
            $devices.value = did
            $devices.setAttribute('dname', $devices.querySelector(`option[value="${did}"]`)?.getAttribute('dname'))
        }
    }
    _getDevice() {
        return this.$devices?.value || this.$devices.querySelector('option[disabled]')?.value
    }

    _intPropsSelect() {
        const { $props } = this
        $props.innerHTML = ''
        let did = this._getDevice()
        if (!did || did == 'null') {
            let nullOpt = document.createElement('option')
            nullOpt.value = 'null'
            nullOpt.innerHTML = 'Select a device first'
            nullOpt.disabled = true
            nullOpt.selected = true
            $props.prepend(nullOpt)
            return
        }

        const device = Devices.find(device => device.did === did)
        if (!device) return
        device.props_name.split(',').forEach(prop => {
            let option = document.createElement('option')
            option.value = prop
            option.innerHTML = prop
            $props.appendChild(option)
        })
    }
    _propSelect(prop) {
        const did = this._getDevice()
        if (!did || did == 'null') return

        const { $props } = this
        if (!prop) {
            let nullOpt = document.createElement('option')
            nullOpt.value = 'null'
            nullOpt.innerHTML = 'Select a property'
            nullOpt.disabled = true
            nullOpt.selected = true
            $props.prepend(nullOpt)
            return
        }
        $props.querySelectorAll('option').forEach(opt => opt.removeAttribute('selected'))
        $props.querySelector(`[value="${prop}"]`)?.setAttribute('selected', '')
    }
    _getProp() {
        return this.$props.querySelector('option[selected]')?.value
    }

    save() {
        const ndata = {
            did: this.modal.querySelector('[name="did"]').value,
            dname: this.modal.querySelector('[name="did"]').getAttribute('dname'),
            prop: this.modal.querySelector('[name="prop"]').value,
            value: this.modal.querySelector('[name="value"]').value
        }
        for (const key in ndata) {
            if (ndata[key] === undefined || ndata[key] === null || ndata[key] === '') {
                Confirm.show('Please fill all fields', `"${key}" is empty`, 'OK')
                return
            }
        }
        Object.assign(this.data, ndata)
        this.data.lastAction = 'save'
        this.destroy()
    }

    delete() {
        this.data.lastAction = 'delete'
        this.destroy()
    }

    discard() {
        this.data.lastAction = 'discard'
        this.destroy()
    }
}